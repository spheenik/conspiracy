#include "IntroWindow.h"

#ifdef CONSPIRACY_LINUX
#include <stdlib.h>
Display                 *dpy;
Window                  root;
GLint                   att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
XVisualInfo             *vi;
Colormap                cmap;
XSetWindowAttributes    swa;
Window                  win;
GLXContext              glc;
XWindowAttributes       gwa;
XEvent                  xev;

HDC			hDC=NULL;

#else
HDC			hDC=NULL;
HGLRC		hRC=NULL;
HWND		hWnd=NULL;
HINSTANCE	hInstance;
#endif

bool	keys[256];
bool	active=TRUE;

bool    done = false;
bool	mode3d = true;

#ifdef CONSPIRACY_LINUX
#else
MSG msg;
#endif
int xres,yres;

GLvoid KillGLWindow(GLvoid)
{
#ifdef CONSPIRACY_LINUX
    XCloseDisplay(dpy);
#else
	ChangeDisplaySettings(NULL,0);
	ShowCursor(TRUE);
#endif
}

BOOL Intro_CreateWindow(char* title, int width, int height, int bits, bool fullscreenflag, HICON icon, bool aontop)
{
	xres=width;
	yres=height;
#ifdef CONSPIRACY_LINUX
    dpy = XOpenDisplay(NULL);

    if(dpy == NULL) {
        printf("\n\tcannot connect to X server\n\n");
        exit(0);
    }

    root = DefaultRootWindow(dpy);

    vi = glXChooseVisual(dpy, 0, att);

    if(vi == NULL) {
        printf("\n\tno appropriate visual found\n\n");
        exit(0);
    }
    else {
        printf("\n\tvisual %p selected\n", (void *)vi->visualid); /* %p creates hexadecimal output like in glxinfo */
    }

    cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);

    swa.colormap = cmap;
    swa.event_mask = KeyPressMask;

    win = XCreateWindow(dpy, root, 0, 0, xres, yres, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);

    XMapWindow(dpy, win);

    XStoreName(dpy, win, title);

    glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
    glXMakeCurrent(dpy, win, glc);

#else
	GLuint		PixelFormat;
	WNDCLASS	wc;
	DWORD		dwExStyle;
	DWORD		dwStyle;
	RECT		WindowRect;
	WindowRect.left=(long)0;
	WindowRect.right=(long)width;
	WindowRect.top=(long)0;
	WindowRect.bottom=(long)height;

	hInstance			= GetModuleHandle(NULL);
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc		= (WNDPROC) WndProc;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;
	wc.hInstance		= hInstance;
	wc.hIcon			= icon;
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground	= NULL;
	wc.lpszMenuName		= NULL;
	wc.lpszClassName	= "OpenGL";

	RegisterClass(&wc);
	
	if (fullscreenflag)
	{
		DEVMODE dmScreenSettings;
		memset(&dmScreenSettings,0,sizeof(dmScreenSettings));
		dmScreenSettings.dmSize=sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsWidth	= width;
		dmScreenSettings.dmPelsHeight	= height;
		dmScreenSettings.dmBitsPerPel	= bits;
		dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

		ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN);
		dwExStyle=WS_EX_APPWINDOW;
		dwStyle=WS_POPUP;
		ShowCursor(FALSE);
	}
	else
	{
	    dwExStyle=WS_EX_APPWINDOW + WS_EX_WINDOWEDGE;
        dwStyle=WS_OVERLAPPED+ WS_CAPTION+ WS_SYSMENU+WS_MINIMIZEBOX;
	}

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);

    hWnd=CreateWindowEx(	dwExStyle,
								"OpenGL",
								title,
								dwStyle |
								WS_CLIPSIBLINGS |
								WS_CLIPCHILDREN,
								CW_USEDEFAULT, CW_USEDEFAULT,
								WindowRect.right-WindowRect.left,
								WindowRect.bottom-WindowRect.top,
								NULL,
								NULL,
								hInstance,
								NULL);

	static	PIXELFORMATDESCRIPTOR pfd=
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW |
		PFD_SUPPORT_OPENGL |
		PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		bits,
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		32,
		0,
		0,
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};
	
	hDC=GetDC(hWnd);
	PixelFormat=ChoosePixelFormat(hDC,&pfd);
	SetPixelFormat(hDC,PixelFormat,&pfd);
	hRC=wglCreateContext(hDC);
	wglMakeCurrent(hDC,hRC);
	ShowWindow(hWnd,SW_SHOW);
	SetForegroundWindow(hWnd);
	SetFocus(hWnd);
	if (aontop) {
		SetWindowPos(hWnd,HWND_TOPMOST,
			(GetSystemMetrics(SM_CXSCREEN)-width)/2,
			(GetSystemMetrics(SM_CYSCREEN)-height)/2,
			0, 0, SWP_NOSIZE);
	}
#endif

	glEnable(GL_DEPTH_TEST);

	return TRUE;									
}

#ifdef CONSPIRACY_LINUX
void handleXevents() {
    while (XCheckWindowEvent(dpy, win, KeyPressMask, &xev)) {
        if (xev.xkey.keycode == 9) {
            keys[27] = true;
        }
    }
}

void SwapBuffers(HDC hdc) {
    glXSwapBuffers(dpy, win);
}
#else
LRESULT CALLBACK WndProc(	HWND	hWnd,
							UINT	uMsg,
							WPARAM	wParam,
							LPARAM	lParam)
{
	switch (uMsg)						
	{
		case WM_ACTIVATE:				
		{
			active=(!HIWORD(wParam));
			return 0;					
		}

		case WM_SYSCOMMAND:				
		{
			switch (wParam)				
			{
				case SC_SCREENSAVE:		
				case SC_MONITORPOWER:	
				return 0;				
			}
			break;						
		}

		case WM_CLOSE:					
		{
			done = true;
			return 0;					
		}

		case WM_KEYDOWN:				
		{
			keys[wParam] = true;
			return 0;					
		}
	}

	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}
#endif

void switchto2d()
{
	glLoadIdentity();
	glViewport(0, 0, xres, yres);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D (0.0, 800, 600,0.0);
}
