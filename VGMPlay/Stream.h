// Stream.h: Header File for constants and structures related to Sound Output
//

#ifdef WIN32
#include <mmsystem.h>
#else
#define MAX_PATH	PATH_MAX
#endif

// https://www.arduino.cc/en/Tutorial/Foundations/Memory
// Memory
// The ATmega328 has 32 KB (with 0.5 KB occupied by the bootloader). It also has 2 KB of SRAM and 1 KB of EEPROM (which can be read and written with the EEPROM library).

// https://playground.arduino.cc/Code/TimerScheduler/

// The ATmega328 wouldn't be able to benefit from buffering because it is a single thread and we
// want to maximize the sample rate for the audio.

// There https://arduino.stackexchange.com/questions/1813/asynchronous-function-calls-in-arduino-sketch
#define SAMPLESIZE		sizeof(WAVE_16BS)
// #define BUFSIZE_MAX		0x1000		// Maximum Buffer Size in Bytes
// #define AUDIOBUFFERS	200			// Maximum Buffer Count

#define BUFSIZE_MAX		0x10000		// Maximum Buffer Size in Bytes
#define AUDIOBUFFERS	2000			// Maximum Buffer Count
//	Windows:	BUFFERSIZE = SampleRate / 100 * SAMPLESIZE (44100 / 100 * 4 = 1764)
//				1 Audio-Buffer = 10 msec, Min: 5
//				Win95- / WinVista-safe: 500 msec
//	Linux:		BUFFERSIZE = 1 << BUFSIZELD (1 << 11 = 2048)
//				1 Audio-Buffer = 11.6 msec

UINT8 SaveFile(UINT32 FileLen, const void* TempData);
UINT8 SoundLogging(UINT8 Mode);
UINT8 StartStream(UINT8 DeviceID);
UINT8 StopStream(void);
void PauseStream(bool PauseOn);
