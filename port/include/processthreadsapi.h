#ifndef CONSPIRACY_PROCESSTHREADSAPI_H
#define CONSPIRACY_PROCESSTHREADSAPI_H

#include "types.h"

typedef void *LPSECURITY_ATTRIBUTES;
typedef size_t SIZE_T;
typedef DWORD (*LPTHREAD_START_ROUTINE) (LPVOID lpThreadParameter);

#define __drv_aliasesMem

HANDLE CreateThread(
        LPSECURITY_ATTRIBUTES   lpThreadAttributes,
        SIZE_T                  dwStackSize,
        LPTHREAD_START_ROUTINE  lpStartAddress,
        __drv_aliasesMem LPVOID lpParameter,
        DWORD                   dwCreationFlags,
        LPDWORD                 lpThreadId
);

BOOL SetThreadPriority(
        HANDLE hThread,
        int    nPriority
);

#define THREAD_PRIORITY_TIME_CRITICAL 15

#endif //CONSPIRACY_PROCESSTHREADSAPI_H
