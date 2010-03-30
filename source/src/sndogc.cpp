/*  Copyright 2005-2006 Theo Berkau

    This file is part of DeSmuME

    DeSmuME is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <gccore.h>
#include <gctypes.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include "types.h"
#include "SPU.h"
#include "sndogc.h"
#include "debug.h"

int SNDOGCInit(int buffersize);
void SNDOGCDeInit();
void SNDOGCUpdateAudio(s16 *buffer, u32 num_samples);
u32 SNDOGCGetAudioSpace();
void SNDOGCMuteAudio();
void SNDOGCUnMuteAudio();
void SNDOGCSetVolume(int volume);

SoundInterface_struct SNDOGC = {
SNDCORE_OGC,
"OGC Sound Interface",
SNDOGCInit,
SNDOGCDeInit,
SNDOGCUpdateAudio,
SNDOGCGetAudioSpace,
SNDOGCMuteAudio,
SNDOGCUnMuteAudio,
SNDOGCSetVolume
};

static u16 *stereodata16[2];
static u16 *currentbuff;
static u8 whichab;
static u32 soundoffset;
static u32 soundbufsize;
static lwpq_t audioqueue = LWP_TQUEUE_NULL;
static lwp_t audiothread = LWP_THREAD_NULL;
static mutex_t audiomutex = LWP_MUTEX_NULL;

static void *audio_thread(void*)
{
	LWP_InitQueue(&audioqueue);
	
	while(1)
	{
		LWP_MutexLock(audiomutex);
		currentbuff = stereodata16[whichab];
		whichab ^= 1;
		soundoffset = 0;
		LWP_MutexUnlock(audiomutex);

		LWP_ThreadSleep(audioqueue);
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////////////

static void MixAudio()
{
	AUDIO_StopDMA();
	DCFlushRange(currentbuff, soundbufsize);
	AUDIO_InitDMA((u32) currentbuff, soundbufsize);
	AUDIO_StartDMA();

	LWP_ThreadSignal(audioqueue);
}

//////////////////////////////////////////////////////////////////////////////

int SNDOGCInit(int buffersize)
{
	whichab = 0;
	AUDIO_Init(NULL);

	AUDIO_SetDSPSampleRate(AI_SAMPLERATE_48KHZ);
	AUDIO_RegisterDMACallback(MixAudio);

	soundbufsize = buffersize * sizeof(s16) * 2;
	soundoffset = 0;

	if ((stereodata16[0] = (u16 *)memalign(soundbufsize, 32)) == NULL ||
	    (stereodata16[1] = (u16 *)memalign(soundbufsize, 32)) == NULL)
		return -1;

	memset(stereodata16[0], 0, soundbufsize);
	memset(stereodata16[1], 0, soundbufsize);

	currentbuff = stereodata16[1];

	//if (audiothread == LWP_THREAD_NULL)
		//LWP_CreateThread(&audiothread, audio_thread, NULL, NULL, 0, 66);

	if (audiomutex == LWP_MUTEX_NULL)
		LWP_MutexInit(&audiomutex, false);

	return 0;
}

//////////////////////////////////////////////////////////////////////////////

void SNDOGCDeInit()
{
	AUDIO_RegisterDMACallback(NULL);
	AUDIO_StopDMA();

	if (stereodata16[0])
		free(stereodata16[0]);
	if (stereodata16[1])
		free(stereodata16[1]);

	LWP_MutexDestroy(audiomutex);
	audiomutex = LWP_MUTEX_NULL;
}

//////////////////////////////////////////////////////////////////////////////

void SNDOGCUpdateAudio(s16 *buffer, u32 num_samples)
{
	u32 copy1size=0, copy2size=0;

	if ((soundbufsize - soundoffset) < (num_samples * sizeof(s16) * 2))
	{
		copy1size = (soundbufsize - soundoffset);
		copy2size = (num_samples * sizeof(s16) * 2) - copy1size;
	}
	else
	{
		copy1size = (num_samples * sizeof(s16) * 2);
		copy2size = 0;
	}

	memcpy((((u8 *)stereodata16[whichab])+soundoffset), buffer, copy1size);

	if (copy2size)
		memcpy(stereodata16[whichab], ((u8 *)buffer)+copy1size, copy2size);

	soundoffset += copy1size + copy2size;
	soundoffset %= soundbufsize;
}

//////////////////////////////////////////////////////////////////////////////

u32 SNDOGCGetAudioSpace()
{
	return ((soundbufsize - soundoffset) / sizeof(s16) / 2);
}

//////////////////////////////////////////////////////////////////////////////

void SNDOGCMuteAudio()
{
}

//////////////////////////////////////////////////////////////////////////////

void SNDOGCUnMuteAudio()
{
}

//////////////////////////////////////////////////////////////////////////////

void SNDOGCSetVolume(int volume)
{
}

//////////////////////////////////////////////////////////////////////////////
