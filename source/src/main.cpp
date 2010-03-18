/* main.c - this file is part of DeSmuME
 *
 * Copyright (C) 2006,2007 DeSmuME Team
 * Copyright (C) 2007 Pascal Giard (evilynux)
 * Copyright (C) 2009 Yoshihiro (DsonPSP)
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <fat.h>
#include <wiiuse/wpad.h>
#include <SDL/SDL.h>
#include <stdio.h>
#include "MMU.h"
#include "NDSSystem.h"
#include "cflash.h"
#include "debug.h"
#include "sndsdl.h"
#include "ctrlssdl.h"
#include "render3D.h"
#include "gdbstub.h"
#include "FrontEnd.h"
#include "Version.h"
#include "log_console.h"


NDS_header * header;

volatile bool execute = false;

static float nds_screen_size_ratio = 1.0f;

#define NUM_FRAMES_TO_TIME 60

#define FPS_LIMITER_FRAME_PERIOD 8

SDL_Surface * surface;
GXRModeObj *rmode = NULL;


/* this holds some info about our display */
const SDL_VideoInfo *videoInfo;  

/* Flags to pass to SDL_SetVideoMode */
static int sdl_videoFlags = 0;
static int sdl_quit = 0;
static u16 keypad;
  
u8 *GPU_vram[512*192*4];
u8 *GPU_mergeA[256*192*4];
u8 *GPU_mergeB[256*192*4];

SoundInterface_struct *SNDCoreList[] = {
  &SNDDummy,
  //&SNDFile,
  &SNDSDL,
  NULL
};

GPU3DInterface *core3DList[] = {
&gpu3DNull
};

//////////////////////////////////////////////////////////////////
////////////////////// FUNCTION PROTOTYPES ///////////////////////
//////////////////////////////////////////////////////////////////

void init();
void apply_surface(int x, int y,int xx, int yy, SDL_Surface* sourceA, SDL_Surface* sourceB, SDL_Surface* destination, SDL_Rect* clip);
static void HDraw(void);
static void VDraw(void);
void ShowFPS();
void DSExec();
void Pause();
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
  
#ifdef __cplusplus
extern "C"
#endif


int main(int argc, char **argv){
  
	int f;
	struct armcpu_memory_iface *arm9_memio = &arm9_base_memory_iface;
	struct armcpu_memory_iface *arm7_memio = &arm7_base_memory_iface;
	struct armcpu_ctrl_iface *arm9_ctrl_iface;
	struct armcpu_ctrl_iface *arm7_ctrl_iface;
	const char* rom_filename;
  
	init();

	log_console_init(rmode, 0, 20, 20, 400, 400);
	log_console_enable_video(true);
	//log_console_enable_log(true);

	printf("\x1b[2;0H");
	printf("Welcome to DeSmuME Wii!!!\n");
	  
	VIDEO_WaitVSync();

	//Check for the ROM
	printf("Looking for sd:/boot.nds...\n");

	fatInitDefault();
	FILE *fp = NULL;
	fp = fopen("sd:/boot.nds", "rb");
  
  
	if(!fp){
		printf("sd:/boot.nds not found. Returning to loader...");
		sleep(1);
		exit(0);
	}
	printf("sd:/boot.nds found!\n");
  
	VIDEO_WaitVSync();
	
	cflash_disk_image_file = NULL;

	printf("Initializing virtual Nintendo DS...\n");

	// Initialize the DS!
	NDS_Init();
	
	printf("initialization successful!\n");

	for(;;){}
  
	enable_sound = true;

	if ( enable_sound) {
		printf("Setting up for sound...\n");
		SPU_ChangeSoundCore(SNDCORE_SDL, 735 * 4);
	}
  
	rom_filename = "sd:/rom.nds";
 
	printf("Placing ROM into virtual NDS...\n");
	if (NDS_LoadROM("sd:/boot.nds", cflash_disk_image_file) < 0) {
		printf("Error loading sd:/boot.nds\n");
		exit(0);
	}

	execute = true;
  
	printf("Initializing SDL...\n");
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == -1){
		fprintf(stderr, "Error trying to initialize SDL: %s\n",
              SDL_GetError());
		return 1;
    }
  
	/* Fetch the video info */
	videoInfo = SDL_GetVideoInfo( );
	if ( !videoInfo ) {
		fprintf( stderr, "Video query failed: %s\n", SDL_GetError( ) );
		exit( -1);
	}

	/* This checks if hardware blits can be done */
	if ( videoInfo->blit_hw )
		sdl_videoFlags |= SDL_HWACCEL;
		sdl_videoFlags |= SDL_SWSURFACE;
    
	// Re-set the video mode
	if(vertical)
		surface = SDL_SetVideoMode(256, 384, 32, sdl_videoFlags);
	else
		surface = SDL_SetVideoMode(480,272, 32, SDL_ANYFORMAT|SDL_DOUBLEBUF|SDL_HWSURFACE|SDL_HWPALETTE);

    if ( !surface ) {
		fprintf( stderr, "Video mode set failed: %s\n", SDL_GetError( ) );
		exit( -1);
    }
  
    SDL_ShowCursor(SDL_DISABLE);
	
    while(!sdl_quit) {
  
		// Look for queued events and update keypad status
		if(frameskip != 0){
			for(s32 f= 0; f < frameskip; f++)
				DSExec();
		}else{
			DSExec();
		}
		if ( enable_sound) {
			SPU_Emulate_core();
			SPU_Emulate_user();
		}

	}


	SDL_Quit();
	NDS_DeInit();
	return 0;
}



//////////////////////////////////////////////////////////////////
//////////////////////////// FUNCTIONS ///////////////////////////
//////////////////////////////////////////////////////////////////


void init(){
 
    // initialize SDL video. If there was an error SDL shows it on the screen
    if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0 ) {
        fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError() );
		SDL_Delay( 500 );
        exit(EXIT_FAILURE);
    }
    
    // button initialization
    WPAD_Init();
 
    // make sure SDL cleans up before exit
    atexit(SDL_Quit);
    SDL_ShowCursor(SDL_DISABLE);
 
    // create a new window
    surface = SDL_SetVideoMode(640, 480, 16, SDL_DOUBLEBUF);
    if ( !surface ) {
        fprintf(stderr, "Unable to set video: %s\n", SDL_GetError());
		SDL_Delay( 500 );
        exit(EXIT_FAILURE);
    }
	
	rmode = VIDEO_GetPreferredMode(NULL);
	
}
 

void apply_surface(int x, int y,int xx, int yy, SDL_Surface* sourceA, SDL_Surface* sourceB, SDL_Surface* destination, SDL_Rect* clip){
    //Holds offsets
    SDL_Rect offset;
    SDL_Rect offsetz;
    SDL_Surface *ZoomA,*ZoomB;

    //Get offsets
    offset.x = x;
    offset.y = y;
    offsetz.x = xx;
    offsetz.y = yy;

	//Blit
	//ZoomA = zoomSurface(sourceA, 0.93, 0.93, 0);
    SDL_BlitSurface( sourceA, clip, destination, &offset );
	//ZoomB = zoomSurface(sourceB, 0.93, 0.93, 0);
	SDL_BlitSurface( sourceB, clip, destination, &offsetz );
    SDL_Flip(destination);	
	
}

static void HDraw(void) {
	SDL_Surface *rawImage,*subImage;

	u16 *src = (u16*)GPU_screen;
	u16 *dstA = (u16*)GPU_mergeA;
	u16 *dstB = (u16*)GPU_mergeB;
	u16 *dst = (u16*)GPU_vram;

	for(int i=0; i < 256*192; i++){ 
		dstA[i] = src[i];           // MainScreen Hack
		dstB[i] = src[(256*192)+i]; // SubScreen Hack
	}

	rawImage = SDL_CreateRGBSurfaceFrom((void*)&GPU_mergeA, 256,192 , 16, 512, 0x001F, 0x03E0, 0x7C00, 0);
	if(rawImage == NULL) return;
	
	subImage = SDL_CreateRGBSurfaceFrom((void*)&GPU_mergeB, 256,192 , 16, 512, 0x001F, 0x03E0, 0x7C00, 0);
	if(subImage == NULL) return;	

	apply_surface( 0, 40,256, 40, rawImage,subImage, surface, 0);

	return;
}

static void VDraw(void) {
	SDL_Surface *rawImage;

	rawImage = SDL_CreateRGBSurfaceFrom((void*)&GPU_screen, 256, 384, 16, 512, 0x001F, 0x03E0, 0x7C00, 0);
	if(rawImage == NULL) return;

	SDL_BlitSurface(rawImage, 0, surface, 0);

	SDL_UpdateRect(surface, 0, 0, 0, 0);

	SDL_FreeSurface(rawImage);

	return;
}

void ShowFPS(){
	u32 fps_timing = 0;
	u32 fps_frame_counter = 0;
	u32 fps_previous_time = 0;
	u32 fps_temp_time;
	float fps;

	fps_frame_counter += 1;
	fps_temp_time = SDL_GetTicks();
	fps_timing += fps_temp_time - fps_previous_time;
	fps_previous_time = fps_temp_time;

	if ( fps_frame_counter == NUM_FRAMES_TO_TIME) {
		fps = (float)fps_timing;
		fps /= NUM_FRAMES_TO_TIME * 1000.f;
		fps = 1.0f / fps;
		fps_frame_counter = 0;
		fps_timing = 0;
		//pspDebugScreenSetTextColor(0xffffffff);
		//pspDebugScreenSetXY(0,0);
		//pspDebugScreenPrintf("FPS %f %s Fskip: %d", fps,VERSION,frameskip);
	}
}

void DSExec(){  
	//	sdl_quit = process_ctrls_events( &keypad, NULL, nds_screen_size_ratio);
    
	WPAD_ScanPads();
	
    // Update mouse position and click
    if(WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_A) {
		NDS_setTouchPos(mouse.x, mouse.y);//ir.x, ir.y
	}
	
    if(!WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_A){ 
        NDS_releaseTouch();
        mouse.click = FALSE;
    }
	
	update_keypad(keypad);     /* Update keypad */
	
	int nb = 0;
    
	NDS_exec<TRUE>(nb);

	if(vertical)
		VDraw();
	else
		HDraw();
	
	showfps = 0;

    if(showfps)
		ShowFPS();

}

void Pause(){

	for(;;){
		WPAD_ScanPads();
		if(WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_A)
			break;
	}

}

/*
int module_start(SceSize args, void *argp)
{
	SceUID uid;

	uid = sceKernelCreateThread("DsOnPSP", _main, 32, 0x10000, 0, 0);
	if(uid < 0)
	{
		return 1;
	}
	sceKernelStartThread(uid, args, argp);

	return 0;
}
*/