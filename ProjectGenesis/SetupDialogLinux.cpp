
#include "SetupDialog.h"
#include "resource.h"

SETUPCFG setupcfg;

int OpenSetupDialog(HINSTANCE hInstance) {
    setupcfg.resolution = 5;
    setupcfg.fullscreen = 0;
    setupcfg.music      = 1;
    setupcfg.texturedetail = 2;
    setupcfg.alwaysontop= 0;
	return 1;
}