// Stream.c: C Source File for Sound Output
//

// Thanks to nextvolume for NetBSD support
#define _GNU_SOURCE
#include <stdio.h>
#include "stdbool.h"
#include <stdlib.h>

#ifdef WIN32
#include <windows.h>
#ifdef USE_LIBAO
#error "Sorry, but this doesn't work yet!"
#endif // USE_LIBAO
#else
#include <unistd.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#ifdef USE_LIBAO
#include <ao/ao.h>
#else
#ifdef __NetBSD__
#include <sys/audioio.h>
#elif defined(__linux__)
#include <linux/soundcard.h>
#endif // __NETBSD__
#endif // USE_LIBAO

#endif // WIN32

#include "chips/mamedef.h"	// for UINT8 etc.
#include "VGMPlay.h"	// neccessary for VGMPlay_Intf.h
#include "VGMPlay_Intf.h"
#include "Stream.h"

#ifndef WIN32
typedef struct
{
	UINT16 wFormatTag;
	UINT16 nChannels;
	UINT32 nSamplesPerSec;
	UINT32 nAvgBytesPerSec;
	UINT16 nBlockAlign;
	UINT16 wBitsPerSample;
	UINT16 cbSize;
} WAVEFORMATEX;	// from MSDN Help

#define WAVE_FORMAT_PCM	0x0001

#endif

#ifdef WIN32
static DWORD WINAPI WaveOutThread(void* Arg);
static void BufCheck(void);
#else
void WaveOutCallbackFnc(void);
#endif

UINT16 AUDIOBUFFERU = AUDIOBUFFERS;		// used AudioBuffers

WAVEFORMATEX WaveFmt;
extern UINT32 SampleRate;
extern volatile bool PauseThread;
volatile bool StreamPause;
extern bool ThreadPauseEnable;
extern volatile bool ThreadPauseConfrm;

UINT32 BlockLen;
#ifdef WIN32
static HWAVEOUT hWaveOut;
static WAVEHDR WaveHdrOut[AUDIOBUFFERS];
static HANDLE hWaveOutThread;
//static DWORD WaveOutCallbackThrID;
#else
static INT32 hWaveOut;
#endif
static bool WaveOutOpen;
UINT32 BUFFERSIZE;	// Buffer Size in Bytes
UINT32 SMPL_P_BUFFER;
static char BufferOut[AUDIOBUFFERS][BUFSIZE_MAX];
static volatile bool CloseThread;


bool SoundLog;
static FILE* hFile;
UINT32 SndLogLen;

UINT32 BlocksSent;
UINT32 BlocksPlayed;

char SoundLogFile[MAX_PATH];

#ifdef USE_LIBAO
ao_device* dev_ao;
#endif

INLINE int fputLE32(UINT32 Value, FILE* hFile)
{
#ifdef VGM_LITTLE_ENDIAN
	return fwrite(&Value, 0x04, 1, hFile);
#else
	int RetVal;
	int ResVal;
	
	RetVal = fputc((Value & 0x000000FF) >>  0, hFile);
	RetVal = fputc((Value & 0x0000FF00) >>  8, hFile);
	RetVal = fputc((Value & 0x00FF0000) >> 16, hFile);
	RetVal = fputc((Value & 0xFF000000) >> 24, hFile);
	ResVal = (RetVal != EOF) ? 0x04 : 0x00;
	return ResVal;
#endif
}

INLINE int fputLE16(UINT16 Value, FILE* hFile)
{
#ifdef VGM_LITTLE_ENDIAN
	return fwrite(&Value, 0x02, 1, hFile);
#else
	int RetVal;
	int ResVal;
	
	RetVal = fputc((Value & 0x00FF) >> 0, hFile);
	RetVal = fputc((Value & 0xFF00) >> 8, hFile);
	ResVal = (RetVal != EOF) ? 0x02 : 0x00;
	return ResVal;
#endif
}

UINT8 SaveFile(UINT32 FileLen, const void* TempData)
{
	//char ResultStr[0x100];
	UINT32 DataLen;
	
	if (TempData == NULL)
	{
		switch(FileLen)
		{
		case 0x00000000:
			if (hFile != NULL)
				return 0xD0;	// file already open
			
			SndLogLen = 0;
			hFile = fopen(SoundLogFile,"wb");
			if (hFile == NULL)
				return 0xFF;	// Save Error
			fseek(hFile, 0x00000000, SEEK_SET);
			fputLE32(0x46464952, hFile);	// 'RIFF'
			fputLE32(0x00000000, hFile);	// RIFF chunk length (dummy)
			
			fputLE32(0x45564157, hFile);	// 'WAVE'
			fputLE32(0x20746D66, hFile);	// 'fmt '
			DataLen = 0x00000010;
			fputLE32(DataLen, hFile);		// format chunk legth
			
#ifdef VGM_LITTLE_ENDIAN
			fwrite(&WaveFmt, DataLen, 1, hFile);
#else
			fputLE16(WaveFmt.wFormatTag,		hFile);	// 0x00
			fputLE16(WaveFmt.nChannels,			hFile);	// 0x02
			fputLE32(WaveFmt.nSamplesPerSec,	hFile);	// 0x04
			fputLE32(WaveFmt.nAvgBytesPerSec,	hFile);	// 0x08
			fputLE16(WaveFmt.nBlockAlign,		hFile);	// 0x0C
			fputLE16(WaveFmt.wBitsPerSample,	hFile);	// 0x0E
			//fputLE16(WaveFmt.cbSize, hFile);			// 0x10 (DataLen is 0x10, so leave this out)
#endif
			
			fputLE32(0x61746164, hFile);	// 'data'
			fputLE32(0x00000000, hFile);	// data chunk length (dummy)
			break;
		case 0xFFFFFFFF:
			if (hFile == NULL)
				return 0x80;	// no file opened
			
			DataLen = SndLogLen * SAMPLESIZE;
			
			fseek(hFile, 0x0028, SEEK_SET);
			fputLE32(DataLen, hFile);			// data chunk length
			fseek(hFile, 0x0004, SEEK_SET);
			fputLE32(DataLen + 0x24, hFile);	// RIFF chunk length
			fclose(hFile);
			hFile = NULL;
			break;
		}
	}
	else
	{
		if (hFile == NULL)
			return 0x80;	// no file opened
		
		//fseek(hFile, 0x00000000, SEEK_END);
		//TempVal[0x0] = ftell(hFile);
		//TempVal[0x1] = fwrite(TempData, 1, FileLen, hFile);
#ifdef VGM_LITTLE_ENDIAN
		SndLogLen += fwrite(TempData, SAMPLESIZE, FileLen, hFile);
#else
		{
			UINT32 CurSmpl;
			const UINT16* SmplData;
			
			SmplData = (UINT16*)TempData;
			DataLen = SAMPLESIZE * FileLen / 0x02;
			for (CurSmpl = 0x00; CurSmpl < DataLen; CurSmpl ++)
				SndLogLen += fputLE16(SmplData[CurSmpl], hFile);
		}
#endif
		//sprintf(ResultStr, "Position:\t%d\nBytes written:\t%d\nFile Length:\t%u\nPointer:\t%p",
		//		TempVal[0], TempVal[1], FileLen, TempData);
		//AfxMessageBox(ResultStr);
	}
	
	return 0x00;
}

UINT8 SoundLogging(UINT8 Mode)
{
	UINT8 RetVal;
	
	RetVal = (UINT8)SoundLog;
	switch(Mode)
	{
	case 0x00:
		SoundLog = false;
		break;
	case 0x01:
		SoundLog = true;
		if (WaveOutOpen && hFile == NULL)
			SaveFile(0x00000000, NULL);
		break;
	case 0xFF:
		break;
	default:
		RetVal = 0xA0;
		break;
	}
	
	return RetVal;
}

UINT8 StartStream(UINT8 DeviceID)
{
	printf("StartStream\n");
	UINT32 RetVal;
	UINT16 Cnt;
	HANDLE WaveOutThreadHandle;
	DWORD WaveOutThreadID;
	
	if (WaveOutOpen)
		return 0xD0;	// Thread is already active
	
	// Init Audio
	WaveFmt.wFormatTag = WAVE_FORMAT_PCM;
	WaveFmt.nChannels = 2;
	WaveFmt.nSamplesPerSec = SampleRate;
	WaveFmt.wBitsPerSample = 16;
	WaveFmt.nBlockAlign = WaveFmt.wBitsPerSample * WaveFmt.nChannels / 8;
	WaveFmt.nAvgBytesPerSec = WaveFmt.nSamplesPerSec * WaveFmt.nBlockAlign;
	WaveFmt.cbSize = 0;
	if (DeviceID == 0xFF)
		return 0x00;
	
	BUFFERSIZE = SampleRate / 100 * SAMPLESIZE;
	if (BUFFERSIZE > BUFSIZE_MAX)
		BUFFERSIZE = BUFSIZE_MAX;
	SMPL_P_BUFFER = BUFFERSIZE / SAMPLESIZE;
	if (AUDIOBUFFERU > AUDIOBUFFERS)
		AUDIOBUFFERU = AUDIOBUFFERS;
	
	PauseThread = true;
	ThreadPauseConfrm = false;
	CloseThread = false;
	StreamPause = false;
	
		ThreadPauseEnable = true;
			printf("CreateThread\n");

		WaveOutThreadHandle = CreateThread(NULL, 0x00, &WaveOutThread, NULL, 0x00,
											&WaveOutThreadID);
		if(WaveOutThreadHandle == NULL)
			return 0xC8;		// CreateThread failed
		CloseHandle(WaveOutThreadHandle);
		
		RetVal = waveOutOpen(&hWaveOut, ((UINT)DeviceID - 1), &WaveFmt, 0x00, 0x00, CALLBACK_NULL);

		if(RetVal != MMSYSERR_NOERROR)
	{
		CloseThread = true;
		return 0xC0;		// waveOutOpen failed
	}
	WaveOutOpen = true;
	
	//sprintf(TestStr, "Buffer 0,0:\t%p\nBuffer 0,1:\t%p\nBuffer 1,0:\t%p\nBuffer 1,1:\t%p\n",
	//		&BufferOut[0][0], &BufferOut[0][1], &BufferOut[1][0], &BufferOut[1][1]);
	//AfxMessageBox(TestStr);
	for (Cnt = 0x00; Cnt < AUDIOBUFFERU; Cnt ++)
	{
		WaveHdrOut[Cnt].lpData = BufferOut[Cnt];	// &BufferOut[Cnt][0x00];
		WaveHdrOut[Cnt].dwBufferLength = BUFFERSIZE;
		WaveHdrOut[Cnt].dwBytesRecorded = 0x00;
		WaveHdrOut[Cnt].dwUser = 0x00;
		WaveHdrOut[Cnt].dwFlags = 0x00;
		WaveHdrOut[Cnt].dwLoops = 0x00;
		WaveHdrOut[Cnt].lpNext = NULL;
		WaveHdrOut[Cnt].reserved = 0x00;
		RetVal = waveOutPrepareHeader(hWaveOut, &WaveHdrOut[Cnt], sizeof(WAVEHDR));
		WaveHdrOut[Cnt].dwFlags |= WHDR_DONE;
	}
	
	PauseThread = false;
				printf("StartStream end\n");

	return 0x00;
}

UINT8 StopStream(void)
{
	UINT32 RetVal;
	UINT16 Cnt;
	
	if (! WaveOutOpen)
		return 0xD8;	// Thread is not active
	
	CloseThread = true;
	for (Cnt = 0; Cnt < 100; Cnt ++)
	{
		Sleep(1);
		if (hWaveOutThread == NULL)
			break;
	}
	if (hFile != NULL)
		SaveFile(0xFFFFFFFF, NULL);
	WaveOutOpen = false;
	
	RetVal = waveOutReset(hWaveOut);
	for (Cnt = 0x00; Cnt < AUDIOBUFFERU; Cnt ++)
		RetVal = waveOutUnprepareHeader(hWaveOut, &WaveHdrOut[Cnt], sizeof(WAVEHDR));
	
	RetVal = waveOutClose(hWaveOut);
	if(RetVal != MMSYSERR_NOERROR) {
		return 0xC4;		// waveOutClose failed  -- but why ???

	}
#ifndef USE_LIBAO
#ifdef WIN32

#else
#endif
#else	// ifdef USE_LIBAO
	ao_close(dev_ao);
	
	ao_shutdown();
#endif
	
	return 0x00;
}

void PauseStream(bool PauseOn)
{
	UINT32 RetVal;
	
	if (! WaveOutOpen)
		return;	// Thread is not active
	
#ifdef WIN32
	switch(PauseOn)
	{
	case true:
		RetVal = waveOutPause(hWaveOut);
		break;
	case false:
		RetVal = waveOutRestart(hWaveOut);
		break;
	}
	StreamPause = PauseOn;
#else
	PauseThread = PauseOn;
#endif
	
	return;
}

static DWORD WINAPI WaveOutThread(void* Arg)
{
									printf("WaveOutThread\n");

	UINT16 CurBuf;
	WAVE_16BS* TempBuf;
	UINT32 WrtSmpls;
	bool DidBuffer;	// a buffer was processed
	
	hWaveOutThread = GetCurrentThread();
	
	BlocksSent = 0x00;
	BlocksPlayed = 0x00;
	while(! CloseThread)
	{
		while(PauseThread && ! CloseThread && ! (StreamPause && DidBuffer))
		{
			ThreadPauseConfrm = true;
			Sleep(1);
		}
		if (CloseThread)
			break;
		
		BufCheck();
		DidBuffer = false;
		for (CurBuf = 0x00; CurBuf < AUDIOBUFFERU; CurBuf ++)
		{
			if (WaveHdrOut[CurBuf].dwFlags & WHDR_DONE)
			{
				TempBuf = (WAVE_16BS*)WaveHdrOut[CurBuf].lpData;
				
				if (WaveHdrOut[CurBuf].dwUser & 0x01)
					BlocksPlayed ++;
				else
					WaveHdrOut[CurBuf].dwUser |= 0x01;
				

				WrtSmpls = FillBuffer(TempBuf, SMPL_P_BUFFER);
				
				WaveHdrOut[CurBuf].dwBufferLength = WrtSmpls * SAMPLESIZE;
				waveOutWrite(hWaveOut, &WaveHdrOut[CurBuf], sizeof(WAVEHDR));
				if (SoundLog && hFile != NULL)
					SaveFile(WrtSmpls, TempBuf);
				
				DidBuffer = true;
				BlocksSent ++;
				BufCheck();
				//CurBuf = 0x00;
				//break;
			}
			if (CloseThread)
				break;
		}
		Sleep(1);
	}
	
	hWaveOutThread = NULL;
	return 0x00000000;
}

static void BufCheck(void)
{
	UINT16 CurBuf;
	
	for (CurBuf = 0x00; CurBuf < AUDIOBUFFERU; CurBuf ++)
	{
		if (WaveHdrOut[CurBuf].dwFlags & WHDR_DONE)
		{
			if (WaveHdrOut[CurBuf].dwUser & 0x01)
			{
				WaveHdrOut[CurBuf].dwUser &= ~0x01;
				BlocksPlayed ++;
			}
		}
	}
	
	return;
}
