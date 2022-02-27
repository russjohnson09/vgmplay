// VGMPlayUI.c: C Source File for the Console User Interface

// Note: In order to make MS VC6 NOT crash when using fprintf with stdout, stderr, etc.
//		 if linked to msvcrt.lib, the following project setting is important:
//		 C/C++ -> Code Generation -> Runtime libraries: Multithreaded DLL

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <wchar.h>
#include <ctype.h>	// for toupper
#include <locale.h>	// for setlocale
#include "stdbool.h"
#include <math.h>

#ifdef WIN32
#include <conio.h>
#include <windows.h>
#else
#include <limits.h>	// for PATH_MAX
#include <termios.h>
#include <unistd.h>	// for STDIN_FILENO and usleep()
#include <sys/time.h>	// for struct timeval in _kbhit()
#include <signal.h> // for signal()
#include <sys/select.h> // for select()

#define	Sleep(msec)	usleep(msec * 1000)
#define _vsnwprintf	vswprintf
#endif

#define printerr(x)	fprintf(stderr, x)

#include "chips/mamedef.h"

#include "Stream.h"
#include "VGMPlay.h"
#include "VGMPlay_Intf.h"
#include "mmkeys.h"
#include "dbus.h"

#ifdef XMAS_EXTRA
#include "XMasFiles/XMasBonus.h"
#endif
#ifdef WS_DEMO
#include "XMasFiles/SWJ-SQRC01_1C.h"
#endif

#ifndef WIN32
void WaveOutLinuxCallBack(void);
#endif

#ifdef WIN32
#define DIR_CHR		'\\'
#define DIR_STR		"\\"
#define QMARK_CHR	'\"'
#else
#define DIR_CHR		'/'
#define DIR_STR		"/"
#define QMARK_CHR	'\''

#ifndef SHARE_PREFIX
#define SHARE_PREFIX	"/usr/local"
#endif

#endif

#define APP_NAME	"VGM Player"
#define APP_NAME_L	L"VGM Player"


int main(int argc, char* argv[]);
static void RemoveNewLines(char* String);
static void RemoveQuotationMarks(char* String);
char* GetLastDirSeparator(const char* FilePath);
static bool IsAbsolutePath(const char* FilePath);
static char* GetFileExtension(const char* FilePath);
static void StandardizeDirSeparators(char* FilePath);
#ifdef WIN32
static void WinNT_Check(void);
#endif
static char* GetAppFileName(void);
static void cls(void);
#ifndef WIN32
static void changemode(bool);
static int _kbhit(void);
static int _getch(void);
#endif
static INT8 stricmp_u(const char *string1, const char *string2);
static INT8 strnicmp_u(const char *string1, const char *string2, size_t count);
static bool GetBoolFromStr(const char* TextStr);
#if defined(XMAS_EXTRA) || defined(WS_DEMO)
static bool XMas_Extra(char* FileName, bool Mode);
#endif
#ifndef WIN32
static void ConvertCP1252toUTF8(char** DstStr, const char* SrcStr);
#endif
static bool OpenPlayListFile(const char* FileName);
static bool OpenMusicFile(const char* FileName);
extern bool OpenVGMFile(const char* FileName);
extern bool OpenOtherFile(const char* FileName);

static void wprintc(const wchar_t* format, ...);
static void PrintChipStr(UINT8 ChipID, UINT8 SubType, UINT32 Clock);
const wchar_t* GetTagStrEJ(const wchar_t* EngTag, const wchar_t* JapTag);
static void ShowVGMTag(void);

INLINE INT8 sign(double Value);
INLINE long int Round(double Value);
INLINE double RoundSpecial(double Value, double RoundTo);
static void PrintMinSec(UINT32 SamplePos, UINT32 SmplRate);


// Options Variables
extern UINT32 SampleRate;	// Note: also used by some sound cores to
							//       determinate the chip sample rate

extern UINT32 VGMPbRate;
extern UINT32 VGMMaxLoop;
extern UINT32 CMFMaxLoop;
UINT32 FadeTimeN;	// normal fade time
UINT32 FadeTimePL;	// in-playlist fade time
extern UINT32 FadeTime;
UINT32 PauseTimeJ;	// Pause Time for Jingles
UINT32 PauseTimeL;	// Pause Time for Looping Songs
extern UINT32 PauseTime;
static UINT8 Show95Cmds;

extern float VolumeLevel;
extern bool SurroundSound;
extern UINT8 HardStopOldVGMs;
extern bool FadeRAWLog;
static UINT8 LogToWave;
//extern bool FullBufFill;
extern bool PauseEmulate;
extern bool DoubleSSGVol;
static UINT16 ForceAudioBuf;
static UINT8 OutputDevID;

extern UINT8 ResampleMode;	// 00 - HQ both, 01 - LQ downsampling, 02 - LQ both
extern UINT8 CHIP_SAMPLING_MODE;
extern INT32 CHIP_SAMPLE_RATE;

extern UINT16 FMPort;
extern bool UseFM;
extern bool FMForce;
//extern bool FMAccurate;
extern bool FMBreakFade;
extern float FMVol;
extern bool FMOPL2Pan;

extern CHIPS_OPTION ChipOpts[0x02];


extern bool ThreadPauseEnable;
extern volatile bool ThreadPauseConfrm;
extern bool ThreadNoWait;	// don't reset the timer

// set a variable in Stream.c
// extern UINT16 AUDIOBUFFERU;

extern UINT32 SMPL_P_BUFFER;
extern char SoundLogFile[MAX_PATH];

extern UINT8 OPL_MODE;
extern UINT8 OPL_CHIPS;
//extern bool WINNT_MODE;
UINT8 NEED_LARGE_AUDIOBUFS;

extern char* AppPaths[8];
static char AppPathBuffer[MAX_PATH * 2];

static char PLFileBase[MAX_PATH];
char PLFileName[MAX_PATH];
UINT32 PLFileCount;
static char** PlayListFile;
UINT32 CurPLFile;
static UINT8 NextPLCmd;
UINT8 PLMode;	// set to 1 to show Playlist text
static bool FirstInit;
extern bool AutoStopSkip;

static UINT8 lastMMEvent = 0x00;

char VgmFileName[MAX_PATH];
static UINT8 FileMode;
extern VGM_HEADER VGMHead;
extern UINT32 VGMDataLen;
extern UINT8* VGMData;
extern GD3_TAG VGMTag;
static bool PreferJapTag;

extern volatile bool PauseThread;
static bool StreamStarted;

extern float MasterVol;

extern UINT32 VGMPos;
extern INT32 VGMSmplPos;
extern INT32 VGMSmplPlayed;
extern INT32 VGMSampleRate;
extern UINT32 BlocksSent;
extern UINT32 BlocksPlayed;
static bool IsRAWLog;
extern bool EndPlay;
extern bool PausePlay;
extern bool FadePlay;
extern bool ForceVGMExec;
extern UINT8 PlayingMode;

extern UINT32 PlayingTime;

extern UINT32 FadeStart;
extern UINT32 VGMMaxLoopM;
extern UINT32 VGMCurLoop;
extern float VolumeLevelM;
bool ErrorHappened;	// used by VGMPlay.c and VGMPlay_AddFmts.c
extern float FinalVol;
extern bool ResetPBTimer;

#ifndef WIN32
static struct termios oldterm;
static bool termmode;
#endif
static volatile bool sigint = false;

UINT8 CmdList[0x100];

//extern UINT8 DISABLE_YMZ_FIX;
extern UINT8 IsVGMInit;
extern UINT16 Last95Drum;	// for optvgm debugging
extern UINT16 Last95Max;	// for optvgm debugging
extern UINT32 Last95Freq;	// for optvgm debugging

static bool PrintMSHours;

#ifdef WIN32
static BOOL WINAPI signal_handler(DWORD dwCtrlType)
{
	switch(dwCtrlType)
	{
	case CTRL_C_EVENT:		// Ctrl + C
	case CTRL_CLOSE_EVENT:	// close console window via X button
	case CTRL_BREAK_EVENT:	// Ctrl + Break
		sigint = true;
		return TRUE;
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		return FALSE;
	}
	return FALSE;
}
#else
static void signal_handler(int signal)
{
	if(signal == SIGINT)
		sigint = true;
}
#endif

// make WINDOWS=1 && ./vgmplay.exe 
int main(int argc, char* argv[])
{
	int argbase;
	int ErrRet;
	char* AppName;
	char* AppPathPtr;
	const char* StrPtr;
	const char* FileExt;
	UINT8 CurPath;
	UINT32 ChrPos;
	char* DispFileName;
	
	VGMPlay_Init();
	
	// Note: Paths are checked from last to first.
	CurPath = 0x00;
	AppPathPtr = AppPathBuffer;
	
	// Path 2: exe's directory
	AppName = GetAppFileName();	// "C:\VGMPlay\VGMPlay.exe"
	
	// Path 4: working directory ("\0")
	AppPathPtr[0] = '\0';
	AppPaths[CurPath] = AppPathPtr;
	CurPath ++;

	// skip reading ini
	// ReadOptions(AppName);

	VGMPlay_Init2();
	
	ErrRet = 0;
	argbase = 0x01;
	
	printf("\nFile Name:\t");
	strcpy(VgmFileName, "../music/megadrive/01 - [Prologue].vgm");
	// strcpy(VgmFileName, "../music/megadrive/02 - Green Hill Zone.vgm");
	// strcpy(VgmFileName, "../music/megadrive/03 - Komorebi no Musume Reading Girl 1.vgm");

	DispFileName = VgmFileName;

	printf("%s\n", DispFileName);

	// StandardizeDirSeparators(VgmFileName);

	FirstInit = true;
	StreamStarted = false;
	FileExt = GetFileExtension(VgmFileName);

	lastMMEvent = 0x00;
		PLFileCount = 0x00;
		CurPLFile = 0x00;
		// no Play List File
		if (! OpenMusicFile(VgmFileName))
		{
			printerr("Error opening the file!\n");
			if (argv[0][1] == ':')
				_getch();
			ErrRet = 1;
			return ErrRet;
		}
		printf("\nsdfsdf");
		
		ErrorHappened = false;
		FadeTime = FadeTimeN;
		PauseTime = PauseTimeL;
		PrintMSHours = (VGMHead.lngTotalSamples >= 158760000);	// 44100 smpl * 60 sec * 60 min


		ShowVGMTag();
		NextPLCmd = 0x80;

	PlayVGM();

// no device id given.
	// StartStream(0);

	// stream is started in another thread.
	
	while(1)
	{
		
	}
	// ThreadNoWait = false;

		// PlayVGM_UI();
		
		// CloseVGMFile();
	
	
	return ErrRet;
}

static void RemoveNewLines(char* String)
{
	char* StrPtr;
	
	StrPtr = String + strlen(String) - 1;
	while(StrPtr >= String && (UINT8)*StrPtr < 0x20)
	{
		*StrPtr = '\0';
		StrPtr --;
	}
	
	return;
}

static void RemoveQuotationMarks(char* String)
{
	UINT32 StrLen;
	char* EndQMark;
	
	if (String[0x00] != QMARK_CHR)
		return;
	
	StrLen = strlen(String);
	memmove(String, String + 0x01, StrLen);	// Remove first char
	EndQMark = strrchr(String, QMARK_CHR);
	if (EndQMark != NULL)
		*EndQMark = 0x00;	// Remove last Quot.-Mark
	
	return;
}

char* GetLastDirSeparator(const char* FilePath)
{
	char* SepPos1;
	char* SepPos2;
	
	SepPos1 = strrchr(FilePath, '/');
	SepPos2 = strrchr(FilePath, '\\');
	if (SepPos1 < SepPos2)
		return SepPos2;
	else
		return SepPos1;
}

static bool IsAbsolutePath(const char* FilePath)
{
#ifdef WIN32
	if (FilePath[0] == '\0')
		return false;	// empty string
	if (FilePath[1] == ':')
		return true;	// Device Path: C:\path
	if (! strncmp(FilePath, "\\\\", 2))
		return true;	// Network Path: \\computername\path
#else
	if (FilePath[0] == '/')
		return true;	// absolute UNIX path
#endif
	return false;
}

static char* GetFileExtension(const char* FilePath)
{
	char* DirSepPos;
	char* ExtDotPos;
	
	DirSepPos = GetLastDirSeparator(FilePath);
	if (DirSepPos == NULL)
		DirSepPos = (char*)FilePath;
	ExtDotPos = strrchr(DirSepPos, '.');
	if (ExtDotPos == NULL)
		return NULL;
	else
		return ExtDotPos + 1;
}

static void StandardizeDirSeparators(char* FilePath)
{
	char* CurChr;
	
	CurChr = FilePath;
	while(*CurChr != '\0')
	{
		if (*CurChr == '\\' || *CurChr == '/')
			*CurChr = DIR_CHR;
		CurChr ++;
	}
	
	return;
}

#ifdef WIN32
static void WinNT_Check(void)
{
	OSVERSIONINFO VerInf;
	
	VerInf.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&VerInf);
	//WINNT_MODE = (VerInf.dwPlatformId == VER_PLATFORM_WIN32_NT);
	
	/* Following Systems need larger Audio Buffers:
		- Windows 95 (500+ ms)
		- Windows Vista (200+ ms)
	Tested Systems:
		- Windows 95B
		- Windows 98 SE
		- Windows 2000
		- Windows XP (32-bit)
		- Windows Vista (32-bit)
		- Windows 7 (64-bit)
	*/
	
	NEED_LARGE_AUDIOBUFS = 0;
	if (VerInf.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
	{
		if (VerInf.dwMajorVersion == 4 && VerInf.dwMinorVersion == 0)
			NEED_LARGE_AUDIOBUFS = 50;	// Windows 95
	}
	else if (VerInf.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{
		if (VerInf.dwMajorVersion == 6 && VerInf.dwMinorVersion == 0)
			NEED_LARGE_AUDIOBUFS = 20;	// Windows Vista
	}
	
	return;
}
#endif

static char* GetAppFileName(void)
{
	char* AppPath;
	
	AppPath = calloc(MAX_PATH, sizeof(char));
#ifdef WIN32
	GetModuleFileName(NULL, AppPath, MAX_PATH - 1);
#else
	readlink("/proc/self/exe", AppPath, MAX_PATH - 1);
#endif
	
	return AppPath;
}

static void cls(void)
{
#ifdef WIN32
	// CLS-Function from the MSDN Help
	HANDLE hConsole;
	COORD coordScreen = {0, 0};
	BOOL bSuccess;
	DWORD cCharsWritten;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	DWORD dwConSize;
	
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	
	// get the number of character cells in the current buffer
	bSuccess = GetConsoleScreenBufferInfo(hConsole, &csbi);
	
	// fill the entire screen with blanks
	dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
	bSuccess = FillConsoleOutputCharacter(hConsole, (TCHAR)' ', dwConSize, coordScreen,
											&cCharsWritten);
	
	// get the current text attribute
	//bSuccess = GetConsoleScreenBufferInfo(hConsole, &csbi);
	
	// now set the buffer's attributes accordingly
	//bSuccess = FillConsoleOutputAttribute(hConsole, csbi.wAttributes, dwConSize, coordScreen,
	//										&cCharsWritten);
	
	// put the cursor at (0, 0)
	bSuccess = SetConsoleCursorPosition(hConsole, coordScreen);
#else
	int retVal;
	
	retVal = system("clear");
#endif
	
	return;
}

#ifndef WIN32

static void changemode(bool dir)
{
	static struct termios newterm;
	
	if (termmode == dir)
		return;
	
	if (dir)
	{
		newterm = oldterm;
		newterm.c_lflag &= ~(ICANON | ECHO);
		tcsetattr(STDIN_FILENO, TCSANOW, &newterm);
	}
	else
	{
		tcsetattr(STDIN_FILENO, TCSANOW, &oldterm);
	}
	termmode = dir;
	
	return;
}

static int _kbhit(void)
{
	struct timeval tv;
	fd_set rdfs;
	int kbret;
	bool needchg;
	
	needchg = (! termmode);
	if (needchg)
		changemode(true);
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	
	FD_ZERO(&rdfs);
	FD_SET(STDIN_FILENO, &rdfs);
	
	select(STDIN_FILENO + 1, &rdfs, NULL, NULL, &tv);
	kbret = FD_ISSET(STDIN_FILENO, &rdfs);
	if (needchg)
		changemode(false);
	
	return kbret;
}

static int _getch(void)
{
	int ch;
	bool needchg;
	
	needchg = (! termmode);
	if (needchg)
		changemode(true);
	ch = getchar();
	if (needchg)
		changemode(false);
	
	return ch;
}
#endif

static INT8 stricmp_u(const char *string1, const char *string2)
{
	// my own stricmp, because VC++6 doesn't find _stricmp when compiling without
	// standard libraries
	const char* StrPnt1;
	const char* StrPnt2;
	char StrChr1;
	char StrChr2;
	
	StrPnt1 = string1;
	StrPnt2 = string2;
	while(true)
	{
		StrChr1 = toupper(*StrPnt1);
		StrChr2 = toupper(*StrPnt2);
		
		if (StrChr1 < StrChr2)
			return -1;
		else if (StrChr1 > StrChr2)
			return +1;
		if (StrChr1 == 0x00)
			return 0;
		
		StrPnt1 ++;
		StrPnt2 ++;
	}
	
	return 0;
}

static INT8 strnicmp_u(const char *string1, const char *string2, size_t count)
{
	// my own strnicmp, because GCC doesn't seem to have _strnicmp
	const char* StrPnt1;
	const char* StrPnt2;
	char StrChr1;
	char StrChr2;
	size_t CurChr;
	
	StrPnt1 = string1;
	StrPnt2 = string2;
	CurChr = 0x00;
	while(CurChr < count)
	{
		StrChr1 = toupper(*StrPnt1);
		StrChr2 = toupper(*StrPnt2);
		
		if (StrChr1 < StrChr2)
			return -1;
		else if (StrChr1 > StrChr2)
			return +1;
		if (StrChr1 == 0x00)
			return 0;
		
		StrPnt1 ++;
		StrPnt2 ++;
		CurChr ++;
	}
	
	return 0;
}

static bool GetBoolFromStr(const char* TextStr)
{
	if (! stricmp_u(TextStr, "True"))
		return true;
	else if (! stricmp_u(TextStr, "False"))
		return false;
	else
		return strtol(TextStr, NULL, 0) ? true : false;
}

#if defined(XMAS_EXTRA) || defined(WS_DEMO)
static bool XMas_Extra(char* FileName, bool Mode)
{
	char* FileTitle;
	const UINT8* XMasData;
	UINT32 XMasSize;
	FILE* hFile;
	
	if (! Mode)
	{	// Prepare Mode
		FileTitle = NULL;
		XMasData = NULL;
#ifdef XMAS_EXTRA
		if (! stricmp_u(FileName, "WEWISH")
		{
			FileTitle = "WEWISH.CMF";
			XMasSize = sizeof(WEWISH_CMF);
			XMasData = WEWISH_CMF;
		}
		else if (! stricmp_u(FileName, "tim7")
		{
			FileTitle = "lem_tim7.vgz";
			XMasSize = sizeof(TIM7_VGZ);
			XMasData = TIM7_VGZ;
		}
		else if (! stricmp_u(FileName, "jingleb")
		{
			FileTitle = "lxmas_jb.dro";
			XMasSize = sizeof(JB_DRO);
			XMasData = JB_DRO;
		}
		else if (! stricmp_u(FileName, "rudolph")
		{
			FileTitle = "rudolph.dro";
			XMasSize = sizeof(RODOLPH_DRO);
			XMasData = RODOLPH_DRO;
		}
		else if (! stricmp_u(FileName, "clyde"))
		{
			FileTitle = "clyde1_1.dro";
			XMasSize = sizeof(clyde1_1_dro);
			XMasData = clyde1_1_dro;
		}
#elif defined(WS_DEMO)
		if (! stricmp_u(FileName, "wswan"))
		{
			FileTitle = "SWJ-SQRC01_1C.vgz";
			XMasSize = sizeof(FF1ws_1C);
			XMasData = FF1ws_1C;
		}
#endif
		
		if (XMasData)
		{
#ifdef WIN32
			GetEnvironmentVariable("Temp", FileName, MAX_PATH);
#else
			strcpy(FileName, "/tmp");
#endif
			strcat(FileName, DIR_STR);
			if (FileTitle == NULL)
				FileTitle = "XMas.dat";
			strcat(FileName, FileTitle);
			
			hFile = fopen(FileName, "wb");
			if (hFile == NULL)
			{
				FileName[0x00] = '\0';
				printerr("Critical XMas-Error!\n");
				return false;
			}
			fwrite(XMasData, 0x01, XMasSize, hFile);
			fclose(hFile);
		}
		else
		{
			FileName = NULL;
			return false;
		}
	}
	else
	{	// Unprepare Mode
		if (! remove(FileName))
			return false;
		// btw: it's intentional that the user can grab the file from the temp-folder
	}
	
	return true;
}
#endif

#ifndef WIN32
static void ConvertCP1252toUTF8(char** DstStr, const char* SrcStr)
{
	const UINT16 CONV_TBL[0x20] =
	{	0x20AC, 0x0000, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,		// 80-87
		0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x0000, 0x017D, 0x0000,		// 88-8F
		0x0000, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,		// 90-97
		0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x0000, 0x017E, 0x0178};	// 98-9F
	UINT32 StrLen;
	UINT16 UnicodeChr;
	char* DstPtr;
	const unsigned char* SrcPtr;
	
	SrcPtr = (const unsigned char*)SrcStr;
	StrLen = 0x00;
	while(*SrcPtr != '\0')
	{
		if (*SrcPtr < 0x80 || *SrcPtr >= 0xA0)
			UnicodeChr = *SrcPtr;
		else
			UnicodeChr = CONV_TBL[*SrcPtr - 0x80];
		if (UnicodeChr < 0x0080)
			StrLen ++;
		else if (UnicodeChr < 0x0800)
			StrLen += 2;
		else
			StrLen += 3;
		SrcPtr ++;
	}
	
	*DstStr = (char*)malloc((StrLen + 0x01) * sizeof(char));
	SrcPtr = (const unsigned char*)SrcStr;
	DstPtr = *DstStr;
	while(*SrcPtr != '\0')
	{
		if (*SrcPtr < 0x80 || *SrcPtr >= 0xA0)
			UnicodeChr = (unsigned char)*SrcPtr;
		else
			UnicodeChr = CONV_TBL[*SrcPtr - 0x80];
		if (UnicodeChr < 0x0080)
		{
			*DstPtr = UnicodeChr & 0xFF;
			DstPtr ++;
		}
		else if (UnicodeChr < 0x0800)
		{
			DstPtr[0x00] = 0xC0 | ((UnicodeChr >> 6) & 0x1F);
			DstPtr[0x01] = 0x80 | ((UnicodeChr >> 0) & 0x3F);
			DstPtr += 0x02;
		}
		else
		{
			DstPtr[0x00] = 0xE0 | ((UnicodeChr >> 12) & 0x0F);
			DstPtr[0x01] = 0x80 | ((UnicodeChr >>  6) & 0x3F);
			DstPtr[0x02] = 0x80 | ((UnicodeChr >>  0) & 0x3F);
			DstPtr += 0x03;
		}
		SrcPtr ++;
	}
	*DstPtr = '\0';
	
	return;
}
#endif

static bool OpenPlayListFile(const char* FileName)
{
	const char M3UV2_HEAD[] = "#EXTM3U";
	const char M3UV2_META[] = "#EXTINF:";
	const UINT8 UTF8_SIG[] = {0xEF, 0xBB, 0xBF};
	UINT32 METASTR_LEN;
	size_t RetVal;
	
	FILE* hFile;
	UINT32 LineNo;
	bool IsV2Fmt;
	UINT32 PLAlloc;
	char TempStr[0x1000];	// 4096 chars should be enough
	char* RetStr;
	bool IsUTF8;
	
	hFile = fopen(FileName, "rt");
	if (hFile == NULL)
		return false;
	
	RetVal = fread(TempStr, 0x01, 0x03, hFile);
	if (RetVal >= 0x03)
		IsUTF8 = ! memcmp(TempStr, UTF8_SIG, 0x03);
	else
		IsUTF8 = false;
	
	rewind(hFile);
	
	PLAlloc = 0x0100;
	PLFileCount = 0x00;
	LineNo = 0x00;
	IsV2Fmt = false;
	METASTR_LEN = strlen(M3UV2_META);
	PlayListFile = (char**)malloc(PLAlloc * sizeof(char*));
	while(! feof(hFile))
	{
		RetStr = fgets(TempStr, 0x1000, hFile);
		if (RetStr == NULL)
			break;
		//RetStr = strchr(TempStr, 0x0D);
		//if (RetStr)
		//	*RetStr = 0x00;	// remove NewLine-Character
		RetStr = TempStr + strlen(TempStr) - 0x01;
		while(RetStr >= TempStr && *RetStr < 0x20)
		{
			*RetStr = '\0';	// remove NewLine-Characters
			RetStr --;
		}
		if (! strlen(TempStr))
			continue;
		
		if (! LineNo)
		{
			if (! strcmp(TempStr, M3UV2_HEAD))
			{
				IsV2Fmt = true;
				LineNo ++;
				continue;
			}
		}
		if (IsV2Fmt)
		{
			if (! strncmp(TempStr, M3UV2_META, METASTR_LEN))
			{
				// Ignore Metadata of m3u Version 2
				LineNo ++;
				continue;
			}
		}
		
		if (PLFileCount >= PLAlloc)
		{
			PLAlloc += 0x0100;
			PlayListFile = (char**)realloc(PlayListFile, PLAlloc * sizeof(char*));
		}
		
		// TODO:
		//	- supprt UTF-8 m3us under Windows
		//	- force IsUTF8 via Commandline
#ifdef WIN32
		// Windows uses the 1252 Codepage by default
		PlayListFile[PLFileCount] = (char*)malloc((strlen(TempStr) + 0x01) * sizeof(char));
		strcpy(PlayListFile[PLFileCount], TempStr);
#else
		if (! IsUTF8)
		{
			// Most recent Linux versions use UTF-8, so I need to convert all strings.
			ConvertCP1252toUTF8(&PlayListFile[PLFileCount], TempStr);
		}
		else
		{
			PlayListFile[PLFileCount] = (char*)malloc((strlen(TempStr) + 0x01) * sizeof(char));
			strcpy(PlayListFile[PLFileCount], TempStr);
		}
#endif
		StandardizeDirSeparators(PlayListFile[PLFileCount]);
		PLFileCount ++;
		LineNo ++;
	}
	
	fclose(hFile);
	
	RetStr = GetLastDirSeparator(FileName);
	if (RetStr != NULL)
	{
		RetStr ++;
		strncpy(PLFileBase, FileName, RetStr - FileName);
		PLFileBase[RetStr - FileName] = '\0';
		StandardizeDirSeparators(PLFileBase);
	}
	else
	{
		strcpy(PLFileBase, "");
	}
	
	return true;
}

static bool OpenMusicFile(const char* FileName)
{
	if (OpenVGMFile(FileName))
		return true;
	else if (OpenOtherFile(FileName))
		return true;
	
	return false;
}

static void wprintc(const wchar_t* format, ...)
{
	va_list arg_list;
	int RetVal;
	UINT32 BufSize;
	wchar_t* printbuf;
#ifdef WIN32
	UINT32 StrLen;
	char* oembuf;
	DWORD CPMode;
#endif
	
	BufSize = 0x00;
	printbuf = NULL;
	do
	{
		BufSize += 0x100;
		printbuf = (wchar_t*)realloc(printbuf, BufSize * sizeof(wchar_t));
		
		// Note: On Linux every vprintf call needs its own set of va_start/va_end commands.
		//       Under Windows (with VC6) one only one block for all calls works, too.
		va_start(arg_list, format);
		RetVal = _vsnwprintf(printbuf, BufSize - 0x01, format, arg_list);
		va_end(arg_list);
	} while(RetVal == -1 && BufSize < 0x1000);
#ifdef WIN32
	StrLen = wcslen(printbuf);
	
	// This is the only way to print Unicode stuff to the Windows console.
	// No, wprintf doesn't work.
	RetVal = WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), printbuf, StrLen, &CPMode, NULL);
	if (! RetVal)	// call failed (e.g. with ERROR_CALL_NOT_IMPLEMENTED on Win95)
	{
		// fallback to printf with OEM codepage
		oembuf = (char*)malloc(BufSize);
		/*if (GetConsoleOutputCP() == GetOEMCP())
			CPMode = CP_OEMCP;
		else
			CPMode = CP_ACP;*/
		CPMode = GetConsoleOutputCP();
		WideCharToMultiByte(CPMode, 0x00, printbuf, StrLen + 1, oembuf, BufSize, NULL, NULL);
		
		printf("%s", oembuf);
		free(oembuf);
	}
#else
	// on Linux, it's easy
	printf("%ls", printbuf);
#endif
	
	free(printbuf);
	
	return;
}

static void PrintChipStr(UINT8 ChipID, UINT8 SubType, UINT32 Clock)
{
	if (! Clock)
		return;
	
	if (ChipID == 0x00 && (Clock & 0x80000000))
		Clock &= ~0x40000000;
	if (Clock & 0x80000000)
	{
		Clock &= ~0x80000000;
		ChipID |= 0x80;
	}
	
	if (Clock & 0x40000000)
		printf("2x");
	printf("%s, ", GetAccurateChipName(ChipID, SubType));
	
	return;
}

const wchar_t* GetTagStrEJ(const wchar_t* EngTag, const wchar_t* JapTag)
{
	const wchar_t* RetTag;
	
	if (EngTag == NULL || ! wcslen(EngTag))
	{
		RetTag = JapTag;
	}
	else if (JapTag == NULL || ! wcslen(JapTag))
	{
		RetTag = EngTag;
	}
	else
	{
		if (! PreferJapTag)
			RetTag = EngTag;
		else
			RetTag = JapTag;
	}
	
	if (RetTag == NULL)
		return L"";
	else
		return RetTag;
}

static void ShowVGMTag(void)
{
	const wchar_t* TitleTag;
	const wchar_t* GameTag;
	const wchar_t* AuthorTag;
	const wchar_t* SystemTag;
	UINT8 CurChip;
	UINT32 ChpClk;
	UINT8 ChpType;
	INT16 VolMod;
#ifdef SET_CONSOLE_TITLE
	wchar_t TitleStr[0x80];
	UINT32 StrLen;
#endif
	
	TitleTag = GetTagStrEJ(VGMTag.strTrackNameE, VGMTag.strTrackNameJ);
	GameTag = GetTagStrEJ(VGMTag.strGameNameE, VGMTag.strGameNameJ);
	AuthorTag = GetTagStrEJ(VGMTag.strAuthorNameE, VGMTag.strAuthorNameJ);
	SystemTag = GetTagStrEJ(VGMTag.strSystemNameE, VGMTag.strSystemNameJ);
	
#ifdef SET_CONSOLE_TITLE
	// --- Show "Song (Game) - VGM Player" as Console Title ---
	if (! wcslen(TitleTag))
	{
		char* TempPtr1;
		char* TempPtr2;
		
		TempPtr1 = strrchr(VgmFileName, '\\');
		TempPtr2 = strrchr(VgmFileName, '/');
		if (TempPtr1 < TempPtr2)
			TempPtr1 = TempPtr2;
		if (TempPtr1 == NULL)
			TempPtr1 = VgmFileName;
		else
			TempPtr1 ++;
		//strncpy(TitleStr, TempPtr1, 0x70);
		mbstowcs(TitleStr, TempPtr1, 0x7F);
		TitleStr[0x70] = '\0';
	}
	else
	{
#if (defined(_MSC_VER) && _MSC_VER < 1400) || defined(OLD_SWPRINTF)
		swprintf(TitleStr, L"%.*ls", 0x70, TitleTag);
#else
		swprintf(TitleStr, 0x80, L"%.*ls", 0x70, TitleTag);
#endif
	}
	StrLen = wcslen(TitleStr);
	
	if (wcslen(GameTag) && StrLen < 0x6C)
	{
#if (defined(_MSC_VER) && _MSC_VER < 1400) || defined(OLD_SWPRINTF)
		swprintf(TitleStr + StrLen, L" (%.*ls)", 0x70 - 3 - StrLen, GameTag);
#else
		swprintf(TitleStr + StrLen, 0x80, L" (%.*ls)", 0x70 - 3 - StrLen, GameTag);
#endif
		StrLen = wcslen(TitleStr);
	}
	
	wcscat(TitleStr, L" - " APP_NAME_L);
#ifdef WIN32
	SetConsoleTitleW(TitleStr);			// Set Windows Console Title
#else
	printf("\x1B]0;%ls\x07", TitleStr);	// Set xterm/rxvt Terminal Title
#endif
#endif
	
	// --- Display Tag Data ---
	if (VGMHead.bytVolumeModifier <= VOLUME_MODIF_WRAP)
		VolMod = VGMHead.bytVolumeModifier;
	else if (VGMHead.bytVolumeModifier == (VOLUME_MODIF_WRAP + 0x01))
		VolMod = VOLUME_MODIF_WRAP - 0x100;
	else
		VolMod = VGMHead.bytVolumeModifier - 0x100;
	
	wprintc(L"Track Title:\t%ls\n", TitleTag);
	wprintc(L"Game Name:\t%ls\n", GameTag);
	wprintc(L"System:\t\t%ls\n", SystemTag);
	wprintc(L"Composer:\t%ls\n", AuthorTag);
	wprintc(L"Release:\t%ls\n", VGMTag.strReleaseDate);
	printf("Version:\t%X.%02X\t", VGMHead.lngVersion >> 8, VGMHead.lngVersion & 0xFF);
	printf("  Gain:%5.2f\t", pow(2.0, VolMod / (double)0x20));
	printf("Loop: ");
	if (VGMHead.lngLoopOffset)
	{
		UINT32 PbRateMul;
		UINT32 PbRateDiv;
		UINT32 PbSamples;
		
		// calculate samples for correct display with changed playback rate
		if (! VGMPbRate || ! VGMHead.lngRate)
		{
			PbRateMul = 1;
			PbRateDiv = 1;
		}
		else
		{
			PbRateMul = VGMHead.lngRate;
			PbRateDiv = VGMPbRate;
		}
		PbSamples = (UINT32)((UINT64)VGMHead.lngLoopSamples * PbRateMul / PbRateDiv);
		
		printf("Yes (");
		PrintMinSec(PbSamples, VGMSampleRate);
		printf(")\n");
	}
	else
	{
		printf("No\n");
	}
	wprintc(L"VGM by:\t\t%ls\n", VGMTag.strCreator);
	wprintc(L"Notes:\t\t%ls\n", VGMTag.strNotes);
	printf("\n");
	
	printf("Used chips:\t");
	for (CurChip = 0x00; CurChip < CHIP_COUNT; CurChip ++)
	{
		ChpClk = GetChipClock(&VGMHead, CurChip, &ChpType);
		if (ChpClk && GetChipClock(&VGMHead, 0x80 | CurChip, NULL))
			ChpClk |= 0x40000000;
		PrintChipStr(CurChip, ChpType, ChpClk);
	}
	printf("\b\b \n");
	printf("\n");
	
	return;
}


INLINE INT8 sign(double Value)
{
	if (Value > 0.0)
		return 1;
	else if (Value < 0.0)
		return -1;
	else
		return 0;
}

INLINE long int Round(double Value)
{
	// Alternative:	(fabs(Value) + 0.5) * sign(Value);
	return (long int)(Value + 0.5 * sign(Value));
}

INLINE double RoundSpecial(double Value, double RoundTo)
{
	return (long int)(Value / RoundTo + 0.5 * sign(Value)) * RoundTo;
}

static void PrintMinSec(UINT32 SamplePos, UINT32 SmplRate)
{
	float TimeSec;
	UINT16 TimeMin;
	UINT16 TimeHours;
	
	TimeSec = (float)RoundSpecial(SamplePos / (double)SmplRate, 0.01);
	//TimeSec = SamplePos / (float)SmplRate;
	TimeMin = (UINT16)TimeSec / 60;
	TimeSec -= TimeMin * 60;
	if (! PrintMSHours)
	{
		printf("%02hu:%05.2f", TimeMin, TimeSec);
	}
	else
	{
		TimeHours = TimeMin / 60;
		TimeMin %= 60;
		printf("%hu:%02hu:%05.2f", TimeHours, TimeMin, TimeSec);
	}
	
	return;
}
