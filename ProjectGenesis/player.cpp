// Code done by Gargaj

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "fmod/minifmod.h"
#include "samples.h"
#include "synth.h"
#include "player.h"
#include "defines.h"

ZAJCALLBACK cnsSynth_LoaderCallback=NULL;

extern "C" unsigned char xm[];
extern "C" unsigned char patterns[];

int po=NUMPATTERNS*0x40*NUMCHANNELS;

void * noteloadcallback(int pattern, int row, int channel) {
	unsigned char * note=new unsigned char[5];
	unsigned char * ps=patterns+0x40*(pattern*NUMCHANNELS+channel) + row;

	for (int i=0; i<5; i++)	note[i]=ps[po*i];

	return note;
}

void delta_decode(char * d,int s) {
	unsigned char ov=0;
	for (int x=0; x < s; x++) 	{
		int nv = d[x] + ov;
		d[x] = ov = nv;
	}
}

void sampleloadcallback(void *buff, int lenbytes, int numbits, int instno, int sampno) {

	if (cnsSynth_LoaderCallback) cnsSynth_LoaderCallback(instno/(float)(NUMISTRUMENTS-1)+1);

	CSample * sam=new CSample(synthdata[instno]);
	sam->GetFullSampleToXM((char*)buff,lenbytes);
}


int music_len=MUSICSIZE;
int music_pos=64;
char *music_data=(char *)xm;

int memread(void *buffer, int size)
{
	memcpy(buffer, (char *)music_data+music_pos, size);
	music_pos += size;
	
	return size;
}

void memseek(int pos, signed char mode)
{
	if (mode == SEEK_SET) 		 music_pos = pos;
	else if (mode == SEEK_CUR) music_pos += pos;
}

int memtell() { return music_pos; }

extern "C" FMUSIC_MODULE	* mod;
int cnsSynth_LoadMusic() {
#ifdef __DELTA
	delta_decode(music_data,MUSICSIZE);
#endif
	FSOUND_File_Read=memread;
	FSOUND_File_Seek=memseek;
	FSOUND_File_Tell=memtell;

	if (!FSOUND_Init(44100)) return 0;
	mod=FMUSIC_LoadSong(sampleloadcallback,noteloadcallback);
	return 1;
}

void cnsSynth_PlayMusic() {	FMUSIC_PlaySong(mod); }
int cnsSynth_GetSync() { return FMUSIC_GetTime(); }
