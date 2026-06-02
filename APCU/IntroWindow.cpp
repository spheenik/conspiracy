#ifdef CONSPIRACY_LINUX
#define GL_GLEXT_PROTOTYPES
#endif
#include "IntroWindow.h"

#ifdef CONSPIRACY_LINUX
#include <stdlib.h>
#include <GL/glext.h>
Display                 *dpy;
Window                  root;
XVisualInfo             *vi;
Colormap                cmap;
XSetWindowAttributes    swa;
Window                  win;
GLXContext              glc;
XWindowAttributes       gwa;
XEvent                  xev;

// The intro renders into an offscreen FBO at the logical xres x yres, then
// SwapBuffers() blits it into the real window preserving aspect (letterboxed),
// so it fills a tiling-WM / oversized window correctly. (Same as ChaosTheory.)
static GLuint   g_fbo = 0, g_fboColor = 0, g_fboDepth = 0;
static int      g_winw = 0, g_winh = 0;     // actual window size, tracked via ConfigureNotify

static void cnsCreateFBO(int w, int h)
{
    glGenFramebuffers(1, &g_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
    glGenRenderbuffers(1, &g_fboColor);
    glBindRenderbuffer(GL_RENDERBUFFER, g_fboColor);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, g_fboColor);
    glGenRenderbuffers(1, &g_fboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, g_fboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, g_fboDepth);
    // leave the FBO bound: all subsequent rendering targets it
}

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

BOOL Intro_CreateWindow(const char* title, int width, int height, int bits, bool fullscreenflag, HICON icon, bool aontop)
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

    GLint att[] = {
            GLX_RGBA,
            GLX_DOUBLEBUFFER,
            GLX_DEPTH_SIZE, 24,
            GLX_RED_SIZE, 8,
            GLX_GREEN_SIZE, 8,
            GLX_BLUE_SIZE, 8,
            GLX_ALPHA_SIZE, 8,
            None };
    vi = glXChooseVisual(dpy, 0, att);

    if(vi == NULL) {
        printf("\n\tno appropriate visual found\n\n");
        exit(0);
    }

    cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);

    swa.colormap = cmap;
    swa.event_mask = KeyPressMask | StructureNotifyMask;

    win = XCreateWindow(dpy, root, 0, 0, xres, yres, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);

    XMapWindow(dpy, win);

    XStoreName(dpy, win, title);

    glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
    glXMakeCurrent(dpy, win, glc);

    g_winw = xres;
    g_winh = yres;          // seeded; corrected by ConfigureNotify once the WM sizes us
    cnsCreateFBO(xres, yres);

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
		//WriteDebug("Always on top.\n");
	}
#endif

	glEnable(GL_DEPTH_TEST);
	//glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	return TRUE;									
}

#ifdef CONSPIRACY_LINUX
void handleXevents() {
    while (XCheckWindowEvent(dpy, win, KeyPressMask | StructureNotifyMask, &xev)) {
        if (xev.type == KeyPress) {
            if (xev.xkey.keycode == 9) keys[27] = true;     // Escape
        } else if (xev.type == ConfigureNotify) {
            g_winw = xev.xconfigure.width;
            g_winh = xev.xconfigure.height;
        }
    }
}

void SwapBuffers(HDC hdc) {
    // Present the offscreen frame into the window, preserving the xres:yres
    // aspect (centered, black bars) regardless of the actual window size.
    glBindFramebuffer(GL_READ_FRAMEBUFFER, g_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    glViewport(0, 0, g_winw, g_winh);
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    float sx = (float)g_winw / (float)xres;
    float sy = (float)g_winh / (float)yres;
    float scale = sx < sy ? sx : sy;
    int vw = (int)(xres * scale);
    int vh = (int)(yres * scale);
    int ox = (g_winw - vw) / 2;
    int oy = (g_winh - vh) / 2;

    glBlitFramebuffer(0, 0, xres, yres, ox, oy, ox + vw, oy + vh, GL_COLOR_BUFFER_BIT, GL_LINEAR);

    glXSwapBuffers(dpy, win);

    glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);   // back to offscreen for the next frame
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
