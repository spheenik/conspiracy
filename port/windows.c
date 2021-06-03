#include <stdlib.h>
#include <windows.h>
#include <time.h>

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
