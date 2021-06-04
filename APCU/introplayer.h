#ifndef __introplayer__
#define __introplayer__

#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "introengine.h"
#include "introwindow.h"

// EFFECTS

void InitEffect(float u1, float v1, float u2, float v2);
void displayframe(int icurrentframe);
float linear(float a, float b, float t);

#endif