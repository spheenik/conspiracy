#ifndef CONSPIRACY_MMSYSTEM_H
#define CONSPIRACY_MMSYSTEM_H

#include <types.h>

DECLARE_HANDLE(HWAVEOUT);
#define WAVE_MAPPER     ((UINT)-1)

typedef struct wavehdr_tag {
    LPSTR              lpData;
    DWORD              dwBufferLength;
    DWORD              dwBytesRecorded;
    DWORD_PTR          dwUser;
    DWORD              dwFlags;
    DWORD              dwLoops;
    struct wavehdr_tag *lpNext;
    DWORD_PTR          reserved;
} WAVEHDR, *PWAVEHDR, *NPWAVEHDR, *LPWAVEHDR;


typedef struct tWAVEFORMATEX {
    WORD  wFormatTag;
    WORD  nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD  nBlockAlign;
    WORD  wBitsPerSample;
    WORD  cbSize;
} WAVEFORMATEX, *PWAVEFORMATEX, *NPWAVEFORMATEX, *LPWAVEFORMATEX;

#define WHDR_DONE       0x00000001  /* done bit */
#define WHDR_PREPARED   0x00000002  /* set if this header has been prepared */
#define WHDR_BEGINLOOP  0x00000004  /* loop start block */
#define WHDR_ENDLOOP    0x00000008  /* loop end block */
#define WHDR_INQUEUE    0x00000010  /* reserved for driver */

#define WAVE_FORMAT_PCM 1

typedef struct mmtime_tag {
    UINT  wType;
    union {
        DWORD  ms;
        DWORD  sample;
        DWORD  cb;
        DWORD  ticks;
        struct {
            BYTE hour;
            BYTE min;
            BYTE sec;
            BYTE frame;
            BYTE fps;
            BYTE dummy;
            BYTE pad[2];
        } smpte;
        struct {
            DWORD songptrpos;
        } midi;
    } u;
} MMTIME, *PMMTIME, *LPMMTIME;

const int TIME_BYTES = 0x0004;

void waveOutPrepareHeader(HWAVEOUT pHwaveout, WAVEHDR *pTag, size_t i);
void waveOutWrite(HWAVEOUT pHwaveout, WAVEHDR *pTag, size_t i);
UINT waveOutOpen(HWAVEOUT *pHwaveout, UINT i, WAVEFORMATEX *pWaveformatex, int i1,int i2, int i3);
void waveOutReset(HWAVEOUT pHwaveout);
void waveOutClose(HWAVEOUT pHwaveout);
void waveOutGetPosition(HWAVEOUT pHwaveout, MMTIME *pTag, size_t i);

#endif //CONSPIRACY_MMSYSTEM_H
