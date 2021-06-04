/******************************************************************************/
/* MINIFMOD.H                                                                 */
/* ----------------                                                           */
/* MiniFMOD public source code release.                                       */
/* This source is provided as-is.  Firelight Multimedia will not support      */
/* or answer questions about the source provided.                             */
/* MiniFMOD Sourcecode is copyright (c) 2000, Firelight Multimedia.           */
/* MiniFMOD Sourcecode is in no way representative of FMOD 3 source.          */
/* Firelight Multimedia is a registered business name.                        */
/* This source must not be redistributed without this notice.                 */
/******************************************************************************/

//==========================================================================================
// MINIFMOD Main header file. Copyright (c), FireLight Multimedia 2000.
// Based on FMOD, copyright (c), FireLight Multimedia 2000.
//==========================================================================================

#ifndef _MINIFMOD_H_
#define _MINIFMOD_H_

#ifdef CONSPIRACY_LINUX
#include <pulse/pulseaudio.h>
#include <pulse/error.h>
#endif


//===============================================================================================
//= DEFINITIONS
//===============================================================================================

//#define __EXCEPTION_HANDLER__
//#define __16BIT_SAMPLE_SUPPORT__

// fmod defined types
typedef struct FMUSIC_MODULE	FMUSIC_MODULE;

//===============================================================================================
//= FUNCTION PROTOTYPES
//===============================================================================================

//#include "music.h"

#ifdef __cplusplus
#define _EXTERN extern "C"
#else
#define _EXTERN extern
#endif

// ==================================
// Initialization / Global functions.
// ==================================
typedef void * (*NOTELOADCALLBACK)(int pattern, int row, int channel);
typedef void (*SAMPLELOADCALLBACK)(void *buff, int lenbytes, int numbits, int instno, int sampno);
//typedef void (*FMUSIC_CALLBACK)(FMUSIC_MODULE *mod, unsigned char param);

// this must be called before FSOUND_Init!
/*
void FSOUND_File_SetCallbacks(int  (*ReadCallback)(void *buffer, int size),
                              void (*SeekCallback)(int pos, signed char mode),
                              int	 (*TellCallback)()
															);
*/
_EXTERN int	 (*FSOUND_File_Read)(void *buffer, int size);
_EXTERN void (*FSOUND_File_Seek)(int pos, signed char mode);
_EXTERN int	 (*FSOUND_File_Tell)();

_EXTERN signed char		FSOUND_Init(int mixrate);
_EXTERN void			FSOUND_Close();

// =============================================================================================
// FMUSIC API
// =============================================================================================

// Song management / playback functions.
// =====================================
//FMUSIC_MODULE *mod;

_EXTERN FMUSIC_MODULE	* FMUSIC_LoadSong(SAMPLELOADCALLBACK sampleloadcallback, NOTELOADCALLBACK noteloadcallback);
_EXTERN signed char	FMUSIC_FreeSong();
_EXTERN signed char	FMUSIC_PlaySong(FMUSIC_MODULE	* mod);
_EXTERN signed char	FMUSIC_StopSong();

// Runtime song information.
// =========================
_EXTERN int				FMUSIC_GetOrder();
_EXTERN int				FMUSIC_GetRow();
_EXTERN unsigned int	FMUSIC_GetTime();
  
#undef _EXTERN

#endif
