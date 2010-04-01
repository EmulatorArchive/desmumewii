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
#include <sys/dir.h>
#include <stdio.h>
#include <unistd.h>
#include "MMU.h"
#include "NDSSystem.h"
#include "cflash.h"
#include "debug.h"
#include "sndogc.h"
#include "ctrlssdl.h"
#include "render3D.h"
#include "gdbstub.h"
#include "FrontEnd.h"
#include "version.h"
#include "log_console.h"
#include "rasterize.h"

#include <ogcsys.h>
#include <sys/time.h>

NDS_header * header;

volatile bool execute = false;

static float nds_screen_size_ratio = 1.0f;

#define NUM_FRAMES_TO_TIME 60

#define FPS_LIMITER_FRAME_PERIOD 8

GXRModeObj *rmode = NULL;

static int sdl_quit = 0;
static u16 keypad;

static unsigned int *xfb[2];
static int currfb;
static GXTexObj TopTex;
#define DEFAULT_FIFO_SIZE (256*1024)
static u8 gp_fifo[DEFAULT_FIFO_SIZE] __attribute__((aligned(32)));
static u16 TopScreen[256*192] __attribute__((aligned(32)));
static GXTexObj BottomTex;
static u16 BottomScreen[256*192] __attribute__((aligned(32)));
static lwp_t vidthread = LWP_THREAD_NULL;
static mutex_t vidmutex = LWP_MUTEX_NULL;
static u8 abort_thread = 0;

SoundInterface_struct *SNDCoreList[] = {
  &SNDDummy,
  //&SNDFile,
  &SNDOGC,
  NULL
};

GPU3DInterface *core3DList[] = {
	&gpu3DNull,
//	&gpu3Dgl,
	&gpu3DRasterize,
	NULL
};

//////////////////////////////////////////////////////////////////
////////////////////// FUNCTION PROTOTYPES ///////////////////////
//////////////////////////////////////////////////////////////////

void init();
static void Draw(void);
void ShowFPS();
void DSExec();
void Pause();
static void *draw_thread(void*);
char* textFileBrowser(const char* directory);
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
  
#ifdef __cplusplus
extern "C"
#endif
void *main_thread(void *arg);

int main(int argc, char **argv)
{
	lwp_t draw_thread;
	LWP_CreateThread(&draw_thread, &main_thread, 0, NULL, 50000, 67);
	while(1);

	return 0;
}

void *main_thread(void *arg)
{	
//	struct armcpu_memory_iface *arm9_memio = &arm9_base_memory_iface;
//	struct armcpu_memory_iface *arm7_memio = &arm7_base_memory_iface;
//	struct armcpu_ctrl_iface *arm9_ctrl_iface;
//	struct armcpu_ctrl_iface *arm7_ctrl_iface;
	const char* rom_filename;
  
	init();

	log_console_init(rmode, 0, 20, 20, 400, 400);
	log_console_enable_video(true);
	//log_console_enable_log(true);

	printf("\x1b[2;0H");
	printf("Welcome to DeSmuME Wii!!!\n");
	  
	VIDEO_WaitVSync();

	fatInitDefault();
  
	VIDEO_WaitVSync();
	
	printf("Pick a ROM:\n");
	
	rom_filename = textFileBrowser("sd:/DSROM");
	
	cflash_disk_image_file = NULL;

	printf("Initializing virtual Nintendo DS...\n");

	// Initialize the DS!
	NDS_Init();
	NDS_3D_ChangeCore(1);
	printf("Initialization successful!\n");

	enable_sound = true;

	if ( enable_sound) {
		printf("Setting up for sound...\n");
		SPU_ChangeSoundCore(SNDCORE_OGC, 2048);
	}
  
	//rom_filename = "sd:/boot.nds";
 
	printf("Placing ROM into virtual NDS...\n");
	if (NDS_LoadROM(rom_filename, cflash_disk_image_file) < 0) {
		printf("Error loading ROM\n");
		exit(0);
	}

	execute = true;

	log_console_enable_video(false);


	if(vidthread == LWP_THREAD_NULL)
		LWP_CreateThread(&vidthread, draw_thread, NULL, NULL, 0, 68);

	while(!sdl_quit)
	{
		// Look for queued events and update keypad status
		if(frameskip != 0){
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

	}

	abort_thread = 1;
	LWP_MutexDestroy(vidmutex);
	vidmutex = LWP_MUTEX_NULL;
	LWP_JoinThread(vidthread, NULL);
	vidthread = LWP_THREAD_NULL;

	NDS_DeInit();

	GX_AbortFrame();
	GX_Flush();

	VIDEO_Flush();
	VIDEO_WaitVSync();
	VIDEO_SetBlack(true);

	exit(0);

	return NULL;
}



//////////////////////////////////////////////////////////////////
//////////////////////////// FUNCTIONS ///////////////////////////
//////////////////////////////////////////////////////////////////


void init(){
	u32 xfbHeight;
	f32 yscale;
	Mtx44 perspective;
	Mtx GXmodelView2D;
	GXColor background = {0, 0, 0, 0xff};
	currfb = 0;

	// button initialization
	PAD_Init();
	WPAD_Init();
 
	VIDEO_Init();
	rmode = VIDEO_GetPreferredMode(NULL);

	switch (rmode->viTVMode >> 2)
	{
		case VI_PAL: // 576 lines (PAL 50hz)
			rmode = &TVPal574IntDfScale;
			rmode->xfbHeight = 480;
			rmode->viYOrigin = (VI_MAX_HEIGHT_PAL - 480)/2;
			rmode->viHeight = 480;
			break;

		case VI_NTSC: // 480 lines (NTSC 60hz)
			break;

		default: // 480 lines (PAL 60Hz)
			break;
	}

	VIDEO_Configure(rmode);

	xfb[0] = (u32 *)MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	xfb[1] = (u32 *)MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

	VIDEO_ClearFrameBuffer(rmode, xfb[0], COLOR_BLACK);
	VIDEO_ClearFrameBuffer(rmode, xfb[1], COLOR_BLACK);
	VIDEO_SetNextFramebuffer (xfb[0]);

	VIDEO_SetBlack(FALSE);

	VIDEO_Flush();
	VIDEO_WaitVSync();
	if (rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();
	else while (VIDEO_GetNextField()) VIDEO_WaitVSync();

	memset(gp_fifo, 0, DEFAULT_FIFO_SIZE);
	GX_Init(gp_fifo, DEFAULT_FIFO_SIZE);

	GX_SetCopyClear(background, 0x00ffffff);
 
	// other gx setup
	GX_SetViewport(0,0,rmode->fbWidth,rmode->efbHeight,0,1);
	yscale = GX_GetYScaleFactor(rmode->efbHeight,rmode->xfbHeight);
	xfbHeight = GX_SetDispCopyYScale(yscale);
	GX_SetScissor(0,0,rmode->fbWidth,rmode->efbHeight);
	GX_SetDispCopySrc(0,0,rmode->fbWidth,rmode->efbHeight);
	GX_SetDispCopyDst(rmode->fbWidth,xfbHeight);
	GX_SetCopyFilter(rmode->aa,rmode->sample_pattern,GX_TRUE,rmode->vfilter);
	GX_SetFieldMode(rmode->field_rendering,((rmode->viHeight==2*rmode->xfbHeight)?GX_ENABLE:GX_DISABLE));

	if (rmode->aa)
		GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
	else
		GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);

	GX_SetCullMode(GX_CULL_NONE);
	GX_CopyDisp(xfb[currfb],GX_TRUE);
	GX_SetDispCopyGamma(GX_GM_1_0);

	GX_SetNumChans(1);
	GX_SetNumTexGens(1);
	GX_SetTevOp(GX_TEVSTAGE0, GX_REPLACE);
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

	GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
	GX_SetAlphaUpdate(GX_TRUE);
	GX_SetColorUpdate(GX_TRUE);

	guOrtho(perspective,0,479,0,639,0,300);
	GX_LoadProjectionMtx(perspective, GX_ORTHOGRAPHIC);

	guMtxIdentity(GXmodelView2D);
	guMtxTransApply (GXmodelView2D, GXmodelView2D, 0.0F, 0.0F, -5.0F);
	GX_LoadPosMtxImm(GXmodelView2D,GX_PNMTX0);

	GX_SetViewport(0,0,rmode->fbWidth,rmode->efbHeight,0,1);
	GX_InvVtxCache();
	GX_ClearVtxDesc();
	GX_InvalidateTexAll();

	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);

	GX_InitTexObj(&TopTex, TopScreen, 256, 192, GX_TF_RGB5A3, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GX_InitTexObj(&BottomTex, BottomScreen, 256, 192, GX_TF_RGB5A3, GX_CLAMP, GX_CLAMP, GX_FALSE);

	memset(TopScreen, 0, 256*192*sizeof(*TopScreen));
	memset(BottomScreen, 0, 256*192*sizeof(*BottomScreen));

	if (vidmutex == LWP_MUTEX_NULL)
		LWP_MutexInit(&vidmutex, false);

	VIDEO_SetBlack(false);
}

#define RGB15_REVERSE(col) ( 0x8000 | (((col) & 0x001F) << 10) | ((col) & 0x03E0)  | (((col) & 0x7C00) >> 10) )

static void Draw(void) {
	// convert to 4x4 textels for GX
	u16 *sTop = (u16*)&GPU_screen;
	u16 *sBottom = sTop+256*192;
	u16 *dTop = TopScreen;
	u16 *dBottom = BottomScreen;
	LWP_MutexLock(vidmutex);
	for (int y = 0; y < 48; y++) {
		for (int h = 0; h < 4; h++) {
			for (int x = 0; x < 64; x++) {
				for (int w = 0; w < 4; w++) {
					*dTop++ = RGB15_REVERSE(sTop[w]);
					*dBottom++ = RGB15_REVERSE(sBottom[w]);
				}
				dTop+=12;     // next tile
				dBottom+=12;
				sTop+=4;
				sBottom+=4;
			}
			dTop-=1020;     // next line
			dBottom-=1020;
		}
		dTop+=1008;       // next row
		dBottom+=1008;
	}

	DCFlushRange(TopScreen, 256*192*2);
	DCFlushRange(BottomScreen, 256*192*2);

	LWP_MutexUnlock(vidmutex);
	
	return;
}

static void *draw_thread(void*)
{
	while(1)
	{
		if (abort_thread)
			break;

		int topX = 40;
		int topY = 40;
		int bottomX, bottomY;
		if (!vertical) {
			topX = 	int((rmode->viWidth /2) - ((256*2) / 2));
			topY = 	int((rmode->viHeight /2) - (192 / 2));
			bottomX = topX + 256;
			bottomY = topY;
		} else {
			topX = 	int((rmode->viWidth /2) - (256 / 2));
			topY = 	int((rmode->viHeight /2) - ((192*2) / 2));
			bottomX = topX;
			bottomY = topY + 192;
		}

		LWP_MutexLock(vidmutex);

		// TOP SCREEN
		GX_LoadTexObj(&TopTex, GX_TEXMAP0);
		GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
			GX_Position2f32(topX, topY);
			GX_TexCoord2f32(0, 0);
			GX_Position2f32(topX, topY+192);
			GX_TexCoord2f32(0, 1);
			GX_Position2f32(topX+256, topY+192);
			GX_TexCoord2f32(1, 1);
			GX_Position2f32(topX+256, topY);
			GX_TexCoord2f32(1, 0);
		GX_End();

		// BOTTOM SCREEN
		GX_LoadTexObj(&BottomTex, GX_TEXMAP0);
		GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
			GX_Position2f32(bottomX, bottomY);
			GX_TexCoord2f32(0, 0);
			GX_Position2f32(bottomX, bottomY+192);
			GX_TexCoord2f32(0, 1);
			GX_Position2f32(bottomX+256, bottomY+192);
			GX_TexCoord2f32(1, 1);
			GX_Position2f32(bottomX+256, bottomY);
			GX_TexCoord2f32(1, 0);
		GX_End();

		GX_DrawDone();

		currfb ^= 1;

		GX_CopyDisp(xfb[currfb],GX_TRUE);
		VIDEO_SetNextFramebuffer(xfb[currfb]);
		VIDEO_Flush();

		LWP_MutexUnlock(vidmutex);

		VIDEO_WaitVSync();
	}

	return NULL;
}

static int64_t gettime(void)
{
	struct timeval tv;
	gettimeofday(&tv,NULL);
	return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

static u32 GetTicks (void)
{
	const u64 ticks      = gettime();
	const u64 ms         = ticks / TB_TIMER_CLOCK;
	return ms;
}

void ShowFPS() {
	u32 fps_timing = 0;
	u32 fps_frame_counter = 0;
	u32 fps_previous_time = 0;
	u32 fps_temp_time;
	float fps;

	fps_frame_counter += 1;
	fps_temp_time = GetTicks();
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
   
	PAD_ScanPads();
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

	if ((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_1) || (PAD_ButtonsDown(0) & PAD_BUTTON_LEFT))
	{
		show_console = !show_console;
		log_console_enable_video(show_console);
	}
	
	if ((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_2) || (PAD_ButtonsDown(0) & PAD_BUTTON_UP))
	{
		vertical = !vertical;
	}


	if((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_HOME) || ((PAD_ButtonsHeld(0) & PAD_TRIGGER_Z) && (PAD_ButtonsHeld(0) & PAD_TRIGGER_R) && (PAD_ButtonsHeld(0) & PAD_TRIGGER_L)))
        exit(0); // meh .. do this for now
	
	
	
	int nb = 0;
    
	NDS_exec<TRUE>(nb);

	Draw();
	
	showfps = 0;

    if(showfps)
		ShowFPS();

}

#define MAXLINES 6

static char buffer[96];
typedef struct {
        char name[MAXPATHLEN];
        int  size;
        int  attr;
} my_dir_ent;

char* textFileBrowser(const char* directory){
        // Set everything up to read
        DIR_ITER* dp = diropen(directory);
        if(!dp){ return NULL; }
        struct stat fstat;
        char filename[MAXPATHLEN];
        int num_entries = 2, i = 0;
        my_dir_ent* dir = (my_dir_ent*)malloc( num_entries * sizeof(dir_ent) );
        // Read each entry of the directory
        while( dirnext(dp, filename, &fstat) == 0 ){
                // Make sure we have room for this one
                if(i == num_entries){
                        ++num_entries;
                        dir = (my_dir_ent*)realloc( dir, num_entries * sizeof(dir_ent) ); 
                }
                strcpy(dir[i].name, filename);
                dir[i].size   = fstat.st_size;
                dir[i].attr   = fstat.st_mode;
                ++i;
        }
        
        dirclose(dp);
        
        int currentSelection = (num_entries > 2) ? 2 : 1;
        while(1){
                printf("\x1b[2J");
				printf("Welcome to DeSmuME Wii!\n\n");
				printf("WARNING! If you paid for this software, you have been scammed!\n\n");
                sprintf(buffer, "browsing %s:\n\n", directory);
                printf(buffer);
                int i = MIN(MAX(0,currentSelection-7),MAX(0,num_entries-14));
                int max = MIN(num_entries, MAX(currentSelection+7,14));
                for(; i<max; ++i){
                        if(i == currentSelection)
                                sprintf(buffer, "*");
                        else    sprintf(buffer, " ");
                        sprintf(buffer, "%s\t%-32s\t%s\n", buffer,
                                dir[i].name, (dir[i].attr&S_IFDIR) ? "DIR" : "");
                        printf(buffer);
                }
                
                /*** Wait for A/up/down press ***/
                while (!(WPAD_ButtonsHeld(0) & WPAD_BUTTON_A) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_A) && !(WPAD_ButtonsHeld(0) & WPAD_BUTTON_UP) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_UP) && !(WPAD_ButtonsHeld(0) & WPAD_BUTTON_DOWN) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN) && !(WPAD_ButtonsHeld(0) & WPAD_BUTTON_HOME) && !(PAD_ButtonsHeld(0) & PAD_TRIGGER_Z)) { PAD_ScanPads(); WPAD_ScanPads(); }
                if((WPAD_ButtonsHeld(0) & WPAD_BUTTON_HOME) || (PAD_ButtonsHeld(0) & PAD_TRIGGER_Z)) exit(0);
				if((WPAD_ButtonsHeld(0) & WPAD_BUTTON_UP) || (PAD_ButtonsHeld(0) & PAD_BUTTON_UP))  currentSelection = (--currentSelection < 0) ? num_entries-1 : currentSelection;
                if((WPAD_ButtonsHeld(0) & WPAD_BUTTON_DOWN) || (PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN)) currentSelection = (currentSelection + 1) % num_entries;			
                if((WPAD_ButtonsHeld(0) & WPAD_BUTTON_A) || (PAD_ButtonsHeld(0) & PAD_BUTTON_A)){
                        if(dir[currentSelection].attr & S_IFDIR){
                                char newDir[MAXPATHLEN];
                                sprintf(newDir, "%s/%s", directory, dir[currentSelection].name);
                                free(dir);
                                printf("\x1b[2J");
                                sprintf(buffer,"MOVING TO %s.\nPress B to continue.\n",newDir);
                                printf(buffer);
                                while (!(WPAD_ButtonsHeld(0) & WPAD_BUTTON_B) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_B)) { PAD_ScanPads(); WPAD_ScanPads(); }
                                return textFileBrowser(newDir);
                        } else {
                                char* newDir = (char*)malloc(MAXPATHLEN);
                                sprintf(newDir, "%s/%s", directory, dir[currentSelection].name);
                                free(dir);
                                printf("\x1b[2J");
                                sprintf(buffer,"SELECTING %s.\nPress B to continue.\n",newDir);
                                printf(buffer);
                                while (!(WPAD_ButtonsHeld(0) & WPAD_BUTTON_B) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_B)) { PAD_ScanPads(); WPAD_ScanPads(); }
                                return newDir;
                        }
                }
                /*** Wait for up/down button release ***/
                while (!(!(WPAD_ButtonsHeld(0) & WPAD_BUTTON_A) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_A) && !(WPAD_ButtonsHeld(0) & WPAD_BUTTON_UP) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_UP) && !(WPAD_ButtonsHeld(0) & WPAD_BUTTON_DOWN) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN))){ PAD_ScanPads(); WPAD_ScanPads(); }
		}	
}


void Pause(){

	for(;;){
		WPAD_ScanPads();
		if(WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_A)
			break;
	}

}
