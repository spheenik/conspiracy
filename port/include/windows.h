// min/max are Win32 macros the engines rely on. Define them OUTSIDE the include
// guard so every #include <windows.h> re-asserts them -- C++ <cmath>/<algorithm>
// pulled in between inclusions can otherwise leave them undefined at the use site.
#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

// rand() (port/windows.c) reproduces MSVC's LCG, which returns 0..0x7fff. The matching
// RAND_MAX must be MSVC's value, not glibc's 2^31 -- otherwise every `rand()/RAND_MAX` in
// the engine (camera shake, starfields, particle scatter, texture noise, ...) collapses to
// ~0. Re-assert it outside the include guard (like min/max) so an intervening <stdlib.h>
// can't restore the wrong value at the use site.
#include <stdlib.h>
#undef RAND_MAX
#define RAND_MAX 0x7fff

#ifndef CONSPIRACY_WINDOWS_H
#define CONSPIRACY_WINDOWS_H

// Minimal Win32 shim for the Linux port. Provides just enough types, macros and
// stub functions for the Conspiracy intros (ProjectGenesis, ChaosTheory) to
// compile against. The actual windowing/timing is implemented in port/windows.c
// and the per-intro Linux window handlers.

#include <stddef.h>

typedef int BOOL;
#define FALSE 0
#define TRUE 1

typedef unsigned char BYTE;
typedef unsigned int UINT;
typedef unsigned long DWORD, *LPDWORD, *DWORD_PTR;
typedef unsigned short WORD;
typedef long LONG;
typedef unsigned long ULONG;
typedef short SHORT;
typedef int INT;
typedef float FLOAT;
typedef char CHAR;
typedef char *LPSTR;
typedef const char *LPCSTR;
typedef void *LPVOID;
typedef const void *LPCVOID;
typedef unsigned int COLORREF;
typedef unsigned short ATOM;

// Pointer-sized integer payloads (32-bit build)
typedef long LRESULT;
typedef unsigned int WPARAM;
typedef long LPARAM;

typedef void *HANDLE;
typedef void *HICON;
typedef void *HINSTANCE;
typedef void *HMODULE;
typedef void *HDC;
typedef void *HWND;
typedef void *HGLRC;
typedef void *HMENU;
typedef void *HBITMAP;
typedef void *HGDIOBJ;
typedef void *HCURSOR;
typedef void *HBRUSH;
typedef void *HFONT;
typedef void *HPALETTE;

#define __stdcall __attribute__((stdcall))
#define WINAPI __stdcall
#define CALLBACK
#define CONST const

#ifndef VK_ESCAPE
#define VK_ESCAPE 27
#endif
#define VK_SNAPSHOT 0x2C

#define MB_OK 0

// MAKEINTRESOURCE: resources don't exist on Linux; just carry the id as a pointer.
#define MAKEINTRESOURCE(i) ((LPSTR)((size_t)((WORD)(i))))

typedef struct tagPOINT { LONG x, y; } POINT, *LPPOINT;
typedef struct tagRECT { LONG left, top, right, bottom; } RECT, *LPRECT;
typedef struct tagSIZE { LONG cx, cy; } SIZE;

typedef struct tagMSG {
    HWND   hwnd;
    UINT   message;
    WPARAM wParam;
    LPARAM lParam;
    DWORD  time;
    POINT  pt;
} MSG, *LPMSG;

// Only the fields the intros actually touch.
typedef struct _devicemode {
    DWORD dmSize;
    DWORD dmFields;
    DWORD dmBitsPerPel;
    DWORD dmPelsWidth;
    DWORD dmPelsHeight;
} DEVMODE;

typedef struct tagPIXELFORMATDESCRIPTOR {
    WORD  nSize;
    WORD  nVersion;
    DWORD dwFlags;
    BYTE  iPixelType;
    BYTE  cColorBits;
    BYTE  cRedBits, cRedShift, cGreenBits, cGreenShift, cBlueBits, cBlueShift;
    BYTE  cAlphaBits, cAlphaShift;
    BYTE  cAccumBits, cAccumRedBits, cAccumGreenBits, cAccumBlueBits, cAccumAlphaBits;
    BYTE  cDepthBits, cStencilBits, cAuxBuffers;
    BYTE  iLayerType;
    BYTE  bReserved;
    DWORD dwLayerMask, dwVisibleMask, dwDamageMask;
} PIXELFORMATDESCRIPTOR;

typedef struct tagBITMAPINFOHEADER {
    DWORD biSize;
    LONG  biWidth, biHeight;
    WORD  biPlanes, biBitCount;
    DWORD biCompression, biSizeImage;
    LONG  biXPelsPerMeter, biYPelsPerMeter;
    DWORD biClrUsed, biClrImportant;
} BITMAPINFOHEADER;

typedef struct tagRGBQUAD { BYTE rgbBlue, rgbGreen, rgbRed, rgbReserved; } RGBQUAD;

typedef struct tagBITMAPINFO {
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD          bmiColors[1];
} BITMAPINFO;

typedef LRESULT (CALLBACK *WNDPROC)(HWND, UINT, WPARAM, LPARAM);

typedef struct tagWNDCLASS {
    UINT      style;
    WNDPROC   lpfnWndProc;
    int       cbClsExtra;
    int       cbWndExtra;
    HINSTANCE hInstance;
    HICON     hIcon;
    HCURSOR   hCursor;
    HBRUSH    hbrBackground;
    LPCSTR    lpszMenuName;
    LPCSTR    lpszClassName;
} WNDCLASS;

#ifdef __cplusplus
extern "C"
{
#endif
DWORD timeGetTime();
void Sleep(DWORD milliseconds);
void *wglGetProcAddress(const char *name);     // -> glXGetProcAddressARB
int  MessageBoxA(HWND hWnd, const char *text, const char *caption, UINT type);
#ifdef __cplusplus
}
#endif

#define MessageBox MessageBoxA

#endif //CONSPIRACY_WINDOWS_H
