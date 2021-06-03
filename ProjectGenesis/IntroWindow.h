#ifndef __IntroWindow__
#define __IntroWindow__

#ifdef CONSPIRACY_LINUX
    #include <stdio.h>
    #include<X11/X.h>
    #include<X11/Xlib.h>
    #include<GL/gl.h>
    #include<GL/glx.h>
    #include<GL/glu.h>
    #include <windows.h>
#else
    #include <windows.h>
    #include <winuser.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>

#ifdef CONSPIRACY_LINUX
extern HDC			hDC;
#else
extern HDC			hDC;
extern HGLRC		hRC;
extern HWND		    hWnd;
extern HINSTANCE	hInstance;
#endif

extern bool	keys[256];
extern bool	active;

extern bool    done;
extern bool	mode3d;

extern GLuint	base,basesmall;
#ifdef CONSPIRACY_LINUX
#else
extern MSG msg;
#endif
extern int xres,yres;

GLvoid KillGLWindow(GLvoid);
#ifdef CONSPIRACY_LINUX
void handleXevents();
void SwapBuffers(HDC hdc);
#else
LRESULT	CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
#endif
BOOL Intro_CreateWindow(char* title, int width, int height, int bits, bool fullscreenflag, HICON icon, bool aontop);
void switchto2d();

#endif