#include "aDDict2Engine.h"
#include "resource.h"
#include "includelist.h"

//#define RELEASETYPE_DEMO

int EventNum;
MATERIAL *MaterialList;
SCENE *SceneList;
WORLD *WorldList;
EVENT *EventList;
PSelect *PolySelectList;

#pragma comment(lib,"winmm.lib")

void Initialize()
{
	AspectRatio=setupcfg.AspectRatio;
#ifdef CONSPIRACY_LINUX
	Intro_CreateWindow("Chaos Theory by Conspiracy",setupcfg.mode,setupcfg.fullscreen==1,0,setupcfg.alwaysontop==1);
#else
	Intro_CreateWindow("Chaos Theory by Conspiracy",setupcfg.mode,setupcfg.fullscreen==1,LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON2)),setupcfg.alwaysontop==1);
#endif
	//WriteDebug("Window Created.\n");
	glClear(0x4100);
	SwapBuffers(hDC);
#ifdef CONSPIRACY_LINUX
	handleXevents();
#else
	if (PeekMessage(&msg,NULL,0,0,PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
#endif
}

#include "Data/project.h"
#include "Data/precalc.h"

#define BINARY_DECLARE(x) \
extern "C" char x[];\
extern "C" int x##_size;

BINARY_DECLARE(music);

#if defined(CONSPIRACY_LINUX)
#include "mvx_lite.h"
#elif defined(RELEASETYPE_DEMO)
#pragma comment(lib,"bass.lib")
#include "bass.h"
int MusicType=0;
HSTREAM str;
#else
#include "mvx_lite.h"
#endif

void PrecalcIntro()
{
	//WriteDebug("Precalc Start.\n");
	glEnable(GL_NORMALIZE);

#ifdef INCLUDE_OBJ_BUTTERFLYSMOOTH
	initVertexBlend();
#endif
	InitTextureBuffer();
	InitIntroEditorEffects();
	
	unsigned char * data= raw_precalc;
	//DisplayPrecalcAnim=false;
	pEventList=0;
	pEventNum=0;
	pEventNum=LoadDataFile((void**)&data, &pMaterialList, &pPolySelectList, &pSceneList, &pWorldList, &pEventList, -1);
	//WriteDebug("Status bar project loaded.\n");

	if (setupcfg.music)
	{
#if defined(CONSPIRACY_LINUX)
		if (!mvxSystem_Init(hWnd,music,music_size)) exit(0);
#elif defined(RELEASETYPE_DEMO)
		if (!BASS_Init(1,44100,0,hWnd,NULL)) exit(0);
		str=BASS_StreamCreateFile(true,music,0,music_size,0);
#else
		if (!mvxSystem_Init(hWnd,music,music_size)) exit(0);
#endif
	}
	//WriteDebug("MVX init done.\n");

	data= raw_project;

	DisplayPrecalcAnim=true;
	EventNum=LoadDataFile((void**)&data, &MaterialList, &PolySelectList, &SceneList, &WorldList, &EventList, -1 /*setupcfg.texturedetail/**/);
	glScissor(0,(YRes-XRes/16*9)/2,XRes,XRes/16*9);
	//WriteDebug("Project loaded.\n");
}

void PlayIntro()
{
	//WriteDebug("Play Start.\n");
	if (setupcfg.music)
	{
#if defined(CONSPIRACY_LINUX)
		mvxSystem_Play();
#elif defined(RELEASETYPE_DEMO)
		BASS_StreamPlay(str,FALSE,0);
#else
		mvxSystem_Play();
#endif
		//WriteDebug("Music Start.\n");
	}
	int StartTime=timeGetTime();
	int Time=0;
	while (!Done)
	{
#ifdef CONSPIRACY_LINUX
		handleXevents();
		if (Keys[VK_ESCAPE]) Done=true;
		else
#else
		if (PeekMessage(&msg,NULL,0,0,PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else if (Keys[VK_ESCAPE]) Done=true;
		else
#endif
		{
			glDisable(GL_SCISSOR_TEST);

			glClear(0x4100);

#if defined(CONSPIRACY_LINUX)
			if (setupcfg.music) Time=(int)(mvxSystem_GetSync()/10.0f);
			else                Time=(int)((timeGetTime()-StartTime)/10);
#elif defined(RELEASETYPE_DEMO)
			if (setupcfg.music) Time=(int)(BASS_ChannelBytes2Seconds(str,BASS_ChannelGetPosition(str))*100.0f);
			else                Time=(int)((timeGetTime()-StartTime)/10);
#else
			if (setupcfg.music) Time=(int)(mvxSystem_GetSync()/10.0f);
			else                Time=(int)((timeGetTime()-StartTime)/10);
#endif
			//Time+=10;
			
			DisplayFrame(Time, MaterialList, SceneList, WorldList, EventList, EventNum);

			SwapBuffers(hDC);
			//Sleep(10);
			//if (Time>10140) Done=true;
		}
	}
}

typedef void (APIENTRY * WGLSWAPINTERVALEXT) (int);

#ifdef CONSPIRACY_LINUX
int main(int argc, char *argv[])
{
	(void)argc; (void)argv;
#else
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow )
{
	//WriteDebug("start.");
	hInstance=hInst;
#endif
/*#ifdef _DEBUG
	setupcfg.fullscreen=0;
	setupcfg.music=0;
	setupcfg.resolution=4;
	setupcfg.texturedetail=2;
	setupcfg.alwaysontop=0;
	setupcfg.cancel=0;
/*#else/**/

	//WriteDebug("cfg done.");
#ifdef CONSPIRACY_LINUX
	if (OpenSetupDialog(0))
#else
	if (OpenSetupDialog(GetModuleHandle(NULL)))
#endif
	{
		Initialize();
		PrecalcIntro();
#ifndef CONSPIRACY_LINUX
		if (setupcfg.vsync)
		{
			WGLSWAPINTERVALEXT wglSwapIntervalEXT = (WGLSWAPINTERVALEXT) wglGetProcAddress("wglSwapIntervalEXT");
			if (wglSwapIntervalEXT) wglSwapIntervalEXT(1); // enable vertical synchronisation
		}
#endif
		PlayIntro();
		if (setupcfg.music)
		{
#if defined(CONSPIRACY_LINUX)
			mvxSystem_Stop();
			mvxSystem_DeInit();
#elif defined(RELEASETYPE_DEMO)
			BASS_ChannelStop(str);
			BASS_StreamFree(str);
#else
			mvxSystem_Stop();
			mvxSystem_DeInit();
#endif
		}
		Intro_DestroyWindow(setupcfg.fullscreen==1);
	}

	return 0;
}