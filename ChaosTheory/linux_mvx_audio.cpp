// ChaosTheory Linux port -- Phase 2 (audio).
//
// Drives the Windows-only mvx softsynth (mvxPlayerLib.lib, converted to ELF by
// convert_mvx.sh) on Linux: provides the CRT / C++ ABI / ACM / DirectSound / thread /
// FPU shims the lib imports, plus a background PulseAudio thread that drains the synth's
// render output to the speakers. The engine just calls mvxSystem_Init/Play/GetSync/Stop.
//
// The synth's own render thread (started by mvxSystem_Play -> CreateThread) renders into a
// fake-DirectSound ring buffer that we back with real memory; the play cursor it polls is
// the REAL consumer position published by the PulseAudio drain thread, so the synth renders
// just ahead of what is actually playing (same clock -> no drift / popping).
#ifdef CONSPIRACY_LINUX
#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include <cmath>
#include <stdint.h>
#include <cstring>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <gsm/gsm.h>

#define STDCALL __attribute__((stdcall))

// ---- C++ ABI ----
extern "C" void* mvx_op_new(unsigned int n)  { return malloc(n); }
extern "C" void  mvx_op_delete(void* p)      { free(p); }
extern "C" void  __purecall()                { abort(); }
extern "C" int   __fltused = 0;

// ---- C runtime (MSVC cdecl names, leading underscore) ----
// _rand/_srand route to the process-wide MSVC LCG in port/windows.c so the synth's noise
// wavetable (osc waveform 3) is built with the same bit-exact RNG the original demo used.
// (glibc rand has RAND_MAX=2^31 vs MSVC 0x7fff, which made the noise table ~65000x too hot
// and pinned the whole mix to flat DC.)
extern "C" int rand(void);
extern "C" void srand(unsigned);
extern "C" {
void*  shim_malloc(size_t n)                         asm("_malloc");
void   shim_free(void* p)                            asm("_free");
double shim_floor(double x)                          asm("_floor");
void*  shim_fopen(const char* p, const char* m)      asm("_fopen");
int    shim_fclose(void* f)                          asm("_fclose");
int    shim_fprintf(void* f, const char* fmt, ...)   asm("_fprintf");
int    shim_vsprintf(char* s, const char* fmt, va_list ap) asm("_vsprintf");
int    shim_rand()                                   asm("_rand");
void   shim_srand(unsigned s)                        asm("_srand");
}
void*  shim_malloc(size_t n) { return malloc(n); }
void   shim_free(void* p)    { free(p); }
double shim_floor(double x)  { return floor(x); }
void*  shim_fopen(const char* p, const char* m) { return fopen(p, m); }
int    shim_fclose(void* f)  { return fclose((FILE*)f); }
int    shim_fprintf(void* f, const char* fmt, ...) { va_list a; va_start(a,fmt); int r=vfprintf((FILE*)f,fmt,a); va_end(a); return r; }
int    shim_vsprintf(char* s, const char* fmt, va_list ap) { return vsprintf(s, fmt, ap); }
int    shim_rand()           { return rand(); }    // MSVC LCG (port/windows.c)
void   shim_srand(unsigned s){ srand(s); }

// ---- ACM -> GSM 6.10 decode ----
// The song's drum/bass instruments are WAVE_FORMAT_GSM610 (tag 0x31, 8kHz mono, WAV49
// 65-byte blocks). mvxPlugSampler::Uncompress decodes them through the Windows ACM API;
// we decode with libgsm and fake the ACM driver model (Uncompress only needs the stream
// Open/Convert/Close path and never validates the driver handles). ACMSTREAMHEADER field
// offsets are the stable ACM ABI: pbSrc@12 cbSrcLength@16 cbSrcLengthUsed@20 pbDst@28
// cbDstLength@32 cbDstLengthUsed@36. All functions return MMSYSERR_NOERROR (0).
extern "C" {
long STDCALL mvx_acmDriverClose(void*, unsigned){ return 0; }
long STDCALL mvx_acmDriverEnum(void*, unsigned long, unsigned long){ return 0; }
long STDCALL mvx_acmDriverOpen(void* phad, void*, unsigned long){ if(phad)*(void**)phad=(void*)1; return 0; }
long STDCALL mvx_acmFormatEnumA(void*, void*, void*, unsigned long, unsigned long){ return 0; }
long STDCALL mvx_acmMetrics(void*, unsigned, void*){ return 0; }
long STDCALL mvx_acmStreamClose(void* has, unsigned long){ if(has) gsm_destroy((gsm)has); return 0; }
long STDCALL mvx_acmStreamPrepareHeader(void*, void*, unsigned long){ return 0; }
long STDCALL mvx_acmStreamOpen(void* phas, void*, void*, void*, void*, unsigned long, unsigned long, unsigned long){
	gsm g = gsm_create();
	int wav49 = 1; gsm_option(g, GSM_OPT_WAV49, &wav49);     // MS-GSM 65-byte block packing
	if (phas) *(void**)phas = g;
	return 0; }
long STDCALL mvx_acmStreamConvert(void* has, void* pash, unsigned long){
	gsm g = (gsm)has;
	unsigned char* h = (unsigned char*)pash;
	unsigned char* src    = *(unsigned char**)(h + 12);
	unsigned int   srclen = *(unsigned int*)  (h + 16);
	unsigned char* dst    = *(unsigned char**)(h + 28);
	unsigned int   dstlen = *(unsigned int*)  (h + 32);
	if (!g || !src || !dst) return 0;
	unsigned int blocks = srclen / 65;                       // each 65-byte block -> 320 samples
	if (blocks * 640 > dstlen) blocks = dstlen / 640;        // 320 samples * 2 bytes, clamp to dst
	gsm_signal* out = (gsm_signal*)dst;
	for (unsigned int b = 0; b < blocks; b++) {
		gsm_decode(g, src + b*65,      out + b*320);         // 1st sub-frame: 33 bytes -> 160
		gsm_decode(g, src + b*65 + 33, out + b*320 + 160);   // 2nd sub-frame: 32 bytes -> 160
	}
	*(unsigned int*)(h + 20) = blocks * 65;                  // cbSrcLengthUsed
	*(unsigned int*)(h + 36) = blocks * 640;                 // cbDstLengthUsed
	return 0; }
}

// the lib's "stop" flag; set by mvxSystem_Stop, watched by the PulseAudio drain thread
extern "C" int mvxSystem_ExitThread;

// ================= DirectSound capture bridge + PulseAudio output =================
static unsigned char*        cap_ring      = 0;     // ring backing the DS secondary buffer
static unsigned int          cap_ringBytes = 0;
static volatile unsigned int cap_play      = 0;     // play cursor (bytes) = real consumer pos
static int                   cap_channels  = 2;
static int                   cap_rate      = 44100;
static int                   cap_bits      = 16;
static volatile int          cap_ready     = 0;
static long                  dbg_locks     = 0;
#define DSOK 0L

static long STDCALL dsb_QI(void* t, void*, void** o){ if(o)*o=t; return DSOK; }
static unsigned long STDCALL dsb_AddRef(void*){ return 1; }
static unsigned long STDCALL dsb_Release(void*){ return 0; }
static long STDCALL dsb_GetCaps(void*, void*){ return DSOK; }
static long STDCALL dsb_GetCurrentPosition(void*, unsigned long* p, unsigned long* w){
	// Play cursor = real consumer position (cap_play), set by the PulseAudio drain thread.
	unsigned int pos = cap_play;
	if(p)*p = pos; if(w)*w = pos;
	return DSOK; }
static long STDCALL dsb_GetFormat(void*, void*, unsigned long, unsigned long* w){ if(w)*w=0; return DSOK; }
static long STDCALL dsb_GetVolume(void*, long* v){ if(v)*v=0; return DSOK; }
static long STDCALL dsb_GetPan(void*, long* v){ if(v)*v=0; return DSOK; }
static long STDCALL dsb_GetFrequency(void*, unsigned long* v){ if(v)*v=cap_rate; return DSOK; }
static long STDCALL dsb_GetStatus(void*, unsigned long* v){ if(v)*v=1/*DSBSTATUS_PLAYING*/; return DSOK; }
static long STDCALL dsb_Initialize(void*, void*, void*){ return DSOK; }
static long STDCALL dsb_Lock(void*, unsigned long off, unsigned long bytes, void** p1, unsigned long* b1, void** p2, unsigned long* b2, unsigned long flags){
	if (flags & 0x1) bytes = cap_ringBytes;             // DSBLOCK_ENTIREBUFFER
	dbg_locks++;
	if (!cap_ringBytes) { if(p1)*p1=0; if(b1)*b1=0; if(p2)*p2=0; if(b2)*b2=0; return DSOK; }
	off %= cap_ringBytes;
	unsigned int first = bytes;
	if (off + first > cap_ringBytes) first = cap_ringBytes - off;
	if(p1)*p1 = cap_ring + off; if(b1)*b1 = first;
	unsigned int rest = bytes - first;
	if (rest) { if(p2)*p2 = cap_ring; if(b2)*b2 = rest; }
	else      { if(p2)*p2 = 0;        if(b2)*b2 = 0; }
	return DSOK; }

// ---- PulseAudio drain thread: ring -> speakers, publishing the consumer position ----
static pthread_t g_pa_thread; static volatile int g_pa_started = 0;
static void* pa_drain(void*) {
	while (!cap_ready) usleep(1000);
	usleep(150000);                                     // let the synth prefill the ring
	pa_sample_spec ss;
	ss.format = (cap_bits == 16) ? PA_SAMPLE_S16LE : PA_SAMPLE_U8;
	ss.rate = cap_rate; ss.channels = cap_channels;
	int err = 0;
	pa_simple* pa = pa_simple_new(NULL, "ChaosTheory", PA_STREAM_PLAYBACK, NULL, "music", &ss, NULL, NULL, &err);
	if (!pa) { fprintf(stderr, "[audio] pa_simple_new: %s\n", pa_strerror(err)); return 0; }
	unsigned int align = (cap_channels * cap_bits) / 8;
	unsigned int CHUNK = (cap_ringBytes / 8) / align * align;   // ~1/8 buffer, frame-aligned
	if (!CHUNK) CHUNK = align;
	unsigned char* buf = (unsigned char*)malloc(CHUNK);
	unsigned int readpos = 0;
	while (!mvxSystem_ExitThread) {
		unsigned int first = CHUNK;
		if (readpos + first > cap_ringBytes) first = cap_ringBytes - readpos;
		memcpy(buf, cap_ring + readpos, first);
		if (first < CHUNK) memcpy(buf + first, cap_ring, CHUNK - first);
		if (pa_simple_write(pa, buf, CHUNK, &err) < 0) break;
		readpos = (readpos + CHUNK) % cap_ringBytes;
		cap_play = readpos;                             // publish consumer pos -> synth renders ahead
	}
	pa_simple_drain(pa, &err);
	pa_simple_free(pa);
	free(buf);
	return 0;
}

static long STDCALL dsb_Play(void*, unsigned long, unsigned long, unsigned long){
	if (!g_pa_started) { g_pa_started = 1; pthread_create(&g_pa_thread, 0, pa_drain, 0); }
	return DSOK; }
static long STDCALL dsb_SetCurrentPosition(void*, unsigned long){ return DSOK; }
static long STDCALL dsb_SetFormat(void*, void*){ return DSOK; }
static long STDCALL dsb_SetVolume(void*, long){ return DSOK; }
static long STDCALL dsb_SetPan(void*, long){ return DSOK; }
static long STDCALL dsb_SetFrequency(void*, unsigned long){ return DSOK; }
static long STDCALL dsb_Stop(void*){ return DSOK; }
static long STDCALL dsb_Unlock(void*, void*, unsigned long, void*, unsigned long){ return DSOK; }
static long STDCALL dsb_Restore(void*){ return DSOK; }
static void* dsb_vtbl[] = {
	(void*)dsb_QI,(void*)dsb_AddRef,(void*)dsb_Release,(void*)dsb_GetCaps,
	(void*)dsb_GetCurrentPosition,(void*)dsb_GetFormat,(void*)dsb_GetVolume,(void*)dsb_GetPan,
	(void*)dsb_GetFrequency,(void*)dsb_GetStatus,(void*)dsb_Initialize,(void*)dsb_Lock,
	(void*)dsb_Play,(void*)dsb_SetCurrentPosition,(void*)dsb_SetFormat,(void*)dsb_SetVolume,
	(void*)dsb_SetPan,(void*)dsb_SetFrequency,(void*)dsb_Stop,(void*)dsb_Unlock,(void*)dsb_Restore };
static void* g_dsb = dsb_vtbl;

static long STDCALL ds_QI(void* t, void*, void** o){ if(o)*o=t; return DSOK; }
static unsigned long STDCALL ds_AddRef(void*){ return 1; }
static unsigned long STDCALL ds_Release(void*){ return 0; }
static long STDCALL ds_CreateSoundBuffer(void*, void* desc, void** o, void*){
	// DSBUFFERDESC: dwSize[0] dwFlags[1] dwBufferBytes[2] dwReserved[3] lpwfxFormat[4]
	unsigned int* d = (unsigned int*)desc;
	unsigned int bytes = d[2];
	if (bytes) {                              // secondary (streaming) buffer
		cap_ring = (unsigned char*)calloc(1, bytes);
		cap_ringBytes = bytes;
		unsigned short* wf = (unsigned short*)(size_t)d[4];   // WAVEFORMATEX*
		if (wf) { cap_channels = wf[1]; cap_rate = *(unsigned int*)(wf+2); cap_bits = wf[7]; }
		cap_ready = 1;
	}
	if(o)*o=&g_dsb; return DSOK; }
static long STDCALL ds_GetCaps(void*, void*){ return DSOK; }
static long STDCALL ds_DuplicateSoundBuffer(void*, void*, void** o){ if(o)*o=&g_dsb; return DSOK; }
static long STDCALL ds_SetCooperativeLevel(void*, void*, unsigned long){ return DSOK; }
static long STDCALL ds_Compact(void*){ return DSOK; }
static long STDCALL ds_GetSpeakerConfig(void*, unsigned long* c){ if(c)*c=0; return DSOK; }
static long STDCALL ds_SetSpeakerConfig(void*, unsigned long){ return DSOK; }
static long STDCALL ds_Initialize(void*, void*){ return DSOK; }
static long STDCALL ds_VerifyCertification(void*, unsigned long* c){ if(c)*c=0; return DSOK; }
static void* ds_vtbl[] = {
	(void*)ds_QI,(void*)ds_AddRef,(void*)ds_Release,(void*)ds_CreateSoundBuffer,
	(void*)ds_GetCaps,(void*)ds_DuplicateSoundBuffer,(void*)ds_SetCooperativeLevel,(void*)ds_Compact,
	(void*)ds_GetSpeakerConfig,(void*)ds_SetSpeakerConfig,(void*)ds_Initialize,(void*)ds_VerifyCertification };
static void* g_ds = ds_vtbl;
extern "C" long STDCALL mvx_DirectSoundCreate8(void*, void** out, void*) { if(out)*out=&g_ds; return DSOK; }

// ================= threads / timing =================
struct ThreadArg { void* proc; void* param; };
static void* thread_tramp(void* a) {
	ThreadArg* ta = (ThreadArg*)a;
	typedef unsigned long (STDCALL *Proc)(void*);
	((Proc)ta->proc)(ta->param);
	free(ta);
	return 0;
}
static void* STDCALL my_CreateThread(void*, unsigned, void* startAddr, void* param, unsigned long, unsigned long* tid) {
	ThreadArg* ta = (ThreadArg*)malloc(sizeof(ThreadArg));
	ta->proc = startAddr; ta->param = param;
	pthread_t t;
	pthread_create(&t, 0, thread_tramp, ta);
	if (tid) *tid = (unsigned long)t;
	return (void*)1;   // non-null fake handle
}
static long STDCALL my_QPC(int64_t* p){ struct timespec ts; clock_gettime(CLOCK_MONOTONIC,&ts); if(p)*p=(int64_t)ts.tv_sec*1000000000LL+ts.tv_nsec; return 1; }
static long STDCALL my_QPF(int64_t* p){ if(p)*p=1000000000LL; return 1; }
static int  STDCALL my_SetThreadPriority(void*, int){ return 1; }
static void STDCALL my_Sleep(unsigned long ms){ usleep(ms ? ms*1000 : 1000); }

extern "C" {
void* mvx_imp_CreateThread              = (void*)&my_CreateThread;
void* mvx_imp_QueryPerformanceCounter   = (void*)&my_QPC;
void* mvx_imp_QueryPerformanceFrequency = (void*)&my_QPF;
void* mvx_imp_SetThreadPriority         = (void*)&my_SetThreadPriority;
void* mvx_imp_Sleep                     = (void*)&my_Sleep;
}
#endif // CONSPIRACY_LINUX
