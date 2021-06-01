#ifndef CONSPIRACY_WINDOWS_H
#define CONSPIRACY_WINDOWS_H

#define CONSPIRACY_LINUX

#include <string.h>
#include <unistd.h>
#include "types.h"
#include "timeapi.h"
#include "processthreadsapi.h"

#define __stdcall __attribute__((stdcall))

#define WINAPI __stdcall
#define CALLBACK __stdcall

DECLARE_HANDLE(HINSTANCE);
DECLARE_HANDLE(HDC);
DECLARE_HANDLE(HGLRC);
DECLARE_HANDLE(HWND);
DECLARE_HANDLE(HICON);

#define Sleep(x) usleep((x)*1000)

HINSTANCE GetModuleHandle (LPCTSTR lpModuleName);

HICON LoadIcon(HINSTANCE hInstance,LPCTSTR lpIconName);

#define MAKEINTRESOURCE(i) ((LPCTSTR)i)

#define VK_ESCAPE (27)

#define TRUE (1)
#define FALSE (0)

#endif //CONSPIRACY_WINDOWS_H
