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
static int sdl_quit = 0;
static u16 keypad;
  
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


int main(int argc, char **argv)
{

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
	fclose(fp);

	printf("sd:/boot.nds found!\n");
  
	VIDEO_WaitVSync();
	
	cflash_disk_image_file = NULL;

	printf("Initializing virtual Nintendo DS...\n");

	// Initialize the DS!
	NDS_Init();
	
	printf("initialization successful!\n");

	enable_sound = true;

	if ( enable_sound) {
		printf("Setting up for sound...\n");
		SPU_ChangeSoundCore(SNDCORE_SDL, 735 * 4);
	}
  
	rom_filename = "sd:/boot.nds";
 
	printf("Placing ROM into virtual NDS...\n");
	if (NDS_LoadROM("sd:/boot.nds", cflash_disk_image_file) < 0) {
		printf("Error loading sd:/boot.nds\n");
		exit(0);
	}

	execute = true;

	
    SDL_ShowCursor(SDL_DISABLE);
	
	SDL_Rect r;

    while(!sdl_quit) 
	{
  
		// clear backbuffer
		r.y = r.x = 0;
		r.w = 640;
		r.h = 480;

	    SDL_FillRect(surface, &r, 0x0);

		// Look for queued events and update keypad status
		if(frameskip != 0)
		{
			for(s32 f= 0; f < frameskip; f++)
				DSExec();
		}else{
			DSExec();
		}
		
		if ( enable_sound) 
		{
			SPU_Emulate_core();
			SPU_Emulate_user();
		}

		// temp BLOCK to show where we are on touchscreen    
		r.x = mouse.x;
		r.y = mouse.y;
		printf("mouse x,y %d %d\n",mouse.x,mouse.y);
		r.h = 5;
		r.w = 5;
		SDL_FillRect(surface, &r, 0xFFFFFF);

		SDL_Flip(surface);	

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
	
}

static void HDraw(void) {
	SDL_Surface *rawImage,*subImage;

	u16 *src = (u16*)GPU_screen;
	u16 *dstA = (u16*)GPU_mergeA;
	u16 *dstB = (u16*)GPU_mergeB;

	for(int i=0; i < 256*192; i++){ 
		dstA[i] = src[i];           // MainScreen Hack
		dstB[i] = src[(256*192)+i]; // SubScreen Hack
	}


	rawImage = SDL_CreateRGBSurfaceFrom((void*)&GPU_mergeA, 256,192 , 16, 512, 0x001F, 0x03E0, 0x7C00, 0);
	if(rawImage == NULL) return;
	
	subImage = SDL_CreateRGBSurfaceFrom((void*)&GPU_mergeB, 256,192 , 16, 512, 0x001F, 0x03E0, 0x7C00, 0);
	if(subImage == NULL) 
	{
		SDL_FreeSurface(rawImage);
		return;	
	}

	apply_surface( 0, 40,256, 40, rawImage,subImage, surface, 0);

	SDL_FreeSurface(rawImage);
	SDL_FreeSurface(subImage);

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


bool show_console = true;
void DSExec(){  
	//	sdl_quit = process_ctrls_events( &keypad, NULL, nds_screen_size_ratio);
    
	WPAD_ScanPads();
	
	process_ctrls_event(&keypad, nds_screen_size_ratio);
	
    // Update mouse position and click
    if(mouse.down) {
		NDS_setTouchPos(mouse.x, mouse.y);//ir.x, ir.y
	}
	
    if(!mouse.down){ 
        NDS_releaseTouch();
        mouse.click = FALSE;
    }

	update_keypad(keypad);     /* Update keypad */

	if (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_1)
	{
		show_console = !show_console;
		log_console_enable_video(show_console);
	}
	
	if (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_2)
	{
		vertical = !vertical;
	}


	if(WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_HOME) exit(0); // meh .. do this for now
	
	
	
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
