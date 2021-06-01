#ifndef __introplayer__
#define __introplayer__

#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "introengine.h"
#include "introwindow.h"

// EFFECTS

#include "cloth.h"

void InitEffect(float u1, float v1, float u2, float v2);
void displayframe(int icurrentframe);

#endif