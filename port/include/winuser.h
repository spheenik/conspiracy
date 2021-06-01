#ifndef CONSPIRACY_WINUSER_H
#define CONSPIRACY_WINUSER_H

#include "types.h"

typedef struct tagPOINT {
    LONG x;
    LONG y;
} POINT, *PPOINT;

typedef struct tagMSG {
    HWND   hwnd;
    UINT   message;
    WPARAM wParam;
    LPARAM lParam;
    DWORD  time;
    POINT  pt;
    DWORD  lPrivate;
} MSG, *PMSG, *NPMSG, *LPMSG;


#define PM_REMOVE 1

BOOL PeekMessage(LPMSG lpMsg,
                 HWND hWnd,
                 UINT wMsgFilterMin,
                 UINT wMsgFilterMax,
                 UINT wRemoveMsg);

void TranslateMessage(MSG *pMsg);
void DispatchMessage(MSG *pMsg);

void SwapBuffers(HDC hdc);

#endif //CONSPIRACY_WINUSER_H
