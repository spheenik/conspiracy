#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <time.h>
#include <unistd.h>
#include <GL/glx.h>

void *wglGetProcAddress(const char *name) {
    return (void *)glXGetProcAddressARB((const GLubyte *)name);
}

int MessageBoxA(HWND hWnd, const char *text, const char *caption, UINT type) {
    (void)hWnd; (void)type;
    fprintf(stderr, "[MessageBox] %s: %s\n", caption ? caption : "", text ? text : "");
    return 1; // IDOK
}

DWORD timeGetTime() {
    struct timespec spec;
    if (clock_gettime(1, &spec) == -1) { /* 1 is CLOCK_MONOTONIC */
        abort();
    }
    return spec.tv_sec * 1000 + spec.tv_nsec / 1e6;
}

inline void Sleep(DWORD milliseconds) {
    usleep(milliseconds * 1000);
}


static unsigned int next = 1;

int rand(void)
{
    next = (next * 214013L + 2531011L);
    return (next >> 16) & 0x7fff;
}

void srand(unsigned int seed)
{
    next = seed;
}
