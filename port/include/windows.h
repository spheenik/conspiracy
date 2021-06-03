#ifndef CONSPIRACY_WINDOWS_H
#define CONSPIRACY_WINDOWS_H

typedef int BOOL;
#define FALSE 0
#define TRUE 1

typedef unsigned char BYTE;
typedef unsigned int UINT;
typedef unsigned long DWORD, *LPDWORD, *DWORD_PTR;
typedef unsigned short WORD;
typedef char *LPSTR;

typedef void *HANDLE;
typedef void *HICON;
typedef void *HINSTANCE;
typedef void *HDC;

#define __stdcall __attribute__((stdcall))
#define WINAPI __stdcall

#define VK_ESCAPE 27

#ifdef __cplusplus
extern "C"
{
#endif
DWORD timeGetTime();
void Sleep(DWORD milliseconds);
#ifdef __cplusplus
}
#endif

#endif //CONSPIRACY_WINDOWS_H
