/*
	Copyright (C) 2006 yopyop
	Copyright (C) 2006-2007 shash
	Copyright (C) 2010 DesmumeWii team

    This file is part of DesmumeWii

    DesmumeWii is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    DesmumeWii is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with DeSmuME; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <queue>
#include <gctypes.h>
#include <gccore.h>
#include "debug.h"

#include "types.h"
#include "debug.h"
#include "MMU.h"
#include "bits.h"
#include "matrix.h"
#include "NDSSystem.h"
#include "GXRender.h"
#include "gfx3d.h"
#include "shaders.h"
#include "texcache.h"

// ------------------- EXTERNAL VARIABLES ------------------
// We need to keep it from continuing whilst we render our 3D 
extern mutex_t vidmutex;
// We need to reset all the variables for video when we're done with 3D
extern GXRModeObj *rmode;

// ----------------------------------------------------------
static CACHE_ALIGN u8 GPU_screen3D[256*192*4];
static CACHE_ALIGN u8 tmp_texture[128*1024*4]; // Needed for temp. GX AAAARRRRGGGGBBB texture

static const unsigned short map3d_cull[4] = {GX_CULL_ALL, GX_CULL_FRONT, GX_CULL_BACK, GX_CULL_NONE};
static const int texEnv[4] = { GX_MODULATE, GX_DECAL, GX_MODULATE, GX_MODULATE };
static const int depthFunc[2] = { GX_LESS, GX_EQUAL };

//Derived values extracted from polyattr etc
static u32 polyID = 0;
static u32 depthFuncMode = GX_EQUAL;
static u32 envMode = 0;
static u32 cullingMask = 0;
static u32 lightMask = 0;
static u32 textureFormat = 0, texturePalette = 0;
static bool alpha31 = false;
static bool alphaDepthWrite;
static bool isTranslucent;

static std::queue<u32> freeTextureIds;
static TexCacheItem* currTexture = NULL;

bool depthupdate;

//------------------------------------------------------------
// Texture Variables (Special thanks to the fine people at gl2gx!)

static Mtx textureView; // When we need to apply a texture, we use this matrix

#define MAX_MIP_LEVEL 10

#define MAX_ARRAY 128
GXTexObj gxtextures[MAX_ARRAY]; // TODO: Make dynamic

// Controls the number of texture units allowed.  Minimum is 8
#define MAX_NO_TEXTURES 8
// Defines the initial number of gxTex slots.  If more than this are needed the memory will be realloc-ed
#define NUM_INIT_TEXT 128
//--DCN: Original number seemed too high:
//#define NUM_INIT_TEXT 4000

typedef struct _gxTex{
	GXTexObj gxObj;
	void *pixels;
	u32 format;

	s32 width;
	s32 height;
	size_t size;

	//u32 glFormat;
	//u8 min_filter;
	u8 base_level;

	u8 max_level;
	
	bool level[MAX_MIP_LEVEL];
	
} gxTex;

// Texture Manager
typedef struct _texManager{
	gxTex * textures;
	u8 * used;
	size_t nTexs;
	size_t usedTexs;
} TexManager;

TexManager texMan;

//------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------
static bool resizeMan(TexManager *texMan, size_t n);
static void initTexManager();

static void activateTex(TexManager *texMan, u32 texID);
static void deleteTex(TexManager *texMan, u32 texID);
static void gxGenTextures( s32 n, u32 *textures );
static void expandFreeTextures();
static void texDeleteCallback(TexCacheItem* item);

static void setTexture(unsigned int format, unsigned int texpal);
static void BeginRenderPoly();
static void InstallPolygonAttrib(unsigned long val);
static void Set3DVideoSettings();

static void ReadFramebuffer();
static char GXInit();
static void GXClose();
static void GXReset();
static void GXRender();
static void GXVramReconfigureSignal();
static void ResetVideoSettings();

//------------------------------------------------------------
// Texture Functions (Special thanks to the fine people at gl2gx!)
//------------------------------------------------------------

//----------------------------------------
//
// Function: resizeMan
//
// Change the capacity of the texture manager
//
// @pre     Make sure there's space to hold the extra textures!
// @post    Extra texture slots will be added to the manager
// @param   texMan: The texture manager to change
// @param   n: The number of textures we want it to hold
// @return  bool: true if everything went well, false if not.
//
//----------------------------------------

bool resizeMan(TexManager *texMan, size_t n){
		
	// Sanity check
	if(texMan->nTexs < 0){
		// If it fails the sanity check, just initialize and move on
		texMan->nTexs = 0;
		texMan->usedTexs = 0;
		texMan->used = NULL;
		texMan->textures = NULL;		
	}
	size_t texManTexs = texMan->nTexs;

	if(texMan->usedTexs > texManTexs){
		texMan->usedTexs = 0;
		for(size_t i = 0; i < texManTexs; i++){
			if(texMan->used[i])
				++texMan->usedTexs;
		}
	}
	
	// If we have NULL pointers, we have no textures
	if(!texMan->textures || !texMan->used){
		texManTexs = texMan->nTexs = 0;
		texMan->usedTexs = 0;
	}
	
	// Manager has more than enough, just return
	if(n < texManTexs){
		return true;
	}
	
	gxTex* new_arr = (gxTex*)realloc(texMan->textures, sizeof(gxTex)*n);
	if(!new_arr){
		//ERROR(OUT_OF_MEMORY);
		return false;
	}
	
	u8* new_used = (u8*)realloc(texMan->used, sizeof(u8)*n);
	
	if(!new_used){
		texMan->textures = new_arr;
		
		//ERROR(OUT_OF_MEMORY);
		return false;
	}
	
	// Mark the new spots as unused
	for(size_t i = texManTexs; i < n; ++i){
		new_used[i] = 0;
	}
	
	texMan->nTexs = n;
	texMan->textures = new_arr;
	texMan->used = new_used;
	
	return true;
}

//----------------------------------------
//
// Function: initTexManager
//
// Initialize and allocate space for our texture manager
//
// @pre     -
// @post    The texture manager has been initialized
// @param   -
// @return  -
//
//----------------------------------------

void initTexManager(){

	texMan = (TexManager){NULL, NULL, 0, 0};
	
	//--DCN: This is a HACK! I am not sure why but if we don't allocate some
	// memory for this beforehand, used will be an uninitialized pointer. 
	// Perhaps I missed something from gl2gx?
	texMan.used = (u8*)malloc(sizeof(u8)*NUM_INIT_TEXT);
	resizeMan(&texMan, NUM_INIT_TEXT);
}

//----------------------------------------
//
// Function: activateTex
//
// Set the texture as being in use and prep it for loading
//
// @pre     -
// @post    The texture is now being "used"
// @param   texMan: The texture manager to add the texture to
// @param   texID: The texture ID of the texture to to add
// @return  -
//
//----------------------------------------

void activateTex(TexManager *texMan, u32 texID){
	size_t i = texID - 1;
	
	if(texMan->used[i])
		return;
	
	texMan->used[i] = 1;

	memset(&texMan->textures[i], 0, sizeof(gxTex));
	
	texMan->textures[i].pixels = NULL;
	texMan->textures[i].width = 0;
	texMan->textures[i].height = 0;
	texMan->textures[i].max_level = 0;

	for(s32 j = 0; j < MAX_MIP_LEVEL; ++j){
		texMan->textures[i].level[j] = false;
	}

	++(texMan->usedTexs);
}

//----------------------------------------
//
// Function: deleteTex
//
// Delete the texture and adjust the texture manager
//
// @pre     -
// @post    The texture is gone! 
// @param   texMan: The texture manager to delete the texture from
// @param   texID: The texture ID of the texture to to delete
// @return  -
//
//----------------------------------------

void deleteTex(TexManager *texMan, u32 texID){

	u32 i = texID - 1;
	
	if(!texMan->used[i])
		return;
	
	gxTex * tex = texMan->textures+i;
	
	if(tex->pixels){
		//TODO: Change to "delete"
		free(tex->pixels);
	}
		
	tex->pixels = NULL;
	tex->width = 0;
	tex->height = 0;

	for(s32 j = 0; j < MAX_MIP_LEVEL; ++j){
		tex->level[j] = false;
	}
		
	texMan->used[i] = 0;
	--(texMan->usedTexs);
	
}

//----------------------------------------
//
// Function: gxGenTextures
//
// Remnant of OGLRender/gl2gx. Generates texture IDs
// NOTE: It might be possible to remove this!
//
// @pre     -
// @post    The texture manager has been initialized
// @param   -
// @return  -
//
//----------------------------------------

void gxGenTextures( s32 n, u32 *textures ){
	//TODO: Change this to "new"
	textures = (u32*)malloc(sizeof(u32)*n);	
	--n;
	do{
		textures[n] = n;
		--n;
	}while(n >= 0);

};

//----------------------------------------
//
// Function: expandFreeTextures
//
// Increases the number of free texture IDs that we can use
// NOTE: Using gxGenTextures and pushing gxTextureID[i] results
//       in complications; gxGenTextures may be unneeded.
//
// @pre     -
// @post    More texture IDs are added to freeTextureIds
// @param   -
// @return  -
//
//----------------------------------------

static void expandFreeTextures(){

	for(u32 i = 0; i < 128; i++)
		freeTextureIds.push(i);
	
	//--DCN: You can try to push gxTextureID,
	//  but I think this is just a remnant of OGL:
	/*
	const s32 kInitTextures = 128;
	u32 gxTextureID[kInitTextures];

	gxGenTextures(kInitTextures, &gxTextureID[0]);

	for(s32 i=0;i<kInitTextures;i++){
		freeTextureIds.push(gxTextureID[i]);
	}
	//*/
}


//----------------------------------------
//
// Function: texDeleteCallback
//
// @pre     -
// @post    -
// @param   item: The texture
// @return  -
//
//----------------------------------------

static void texDeleteCallback(TexCacheItem* item){
	freeTextureIds.push((u32)item->texid);
	if(currTexture == item){
		currTexture = NULL;
	}
}

//----------------------------------------
//
// Function: GXReset
//
// Reset our textures and variables to 0
//
// @pre     -
// @post    -
// @param   -
// @return  -
//
//----------------------------------------

static void GXReset(){

	TexCache_Reset();
	if (currTexture) 
		delete currTexture;
	currTexture = NULL;

	memset(GPU_screen3D,0,sizeof(GPU_screen3D));
}

//----------------------------------------
//
// Function: GXInit
//
// Set up all of our texture caches and variables
//
// @pre     -
// @post    -
// @param   -
// @return  char: Return 1 if everything went well
//
//----------------------------------------

static char GXInit(){

	initTexManager();

	expandFreeTextures();
	
	GXReset();

	return 1;
}

//----------------------------------------
//
// Function: GXClose
//
// Delete all the textures that we've used and clean up after ourselves
//
// @pre     -
// @post    -
// @param   -
// @return  -
//
//----------------------------------------

static void GXClose(){

	// Kill the texture cache to free all of the texture ids
	TexCache_Reset();

	while(!freeTextureIds.empty()){
		u32 temp = freeTextureIds.front();
		freeTextureIds.pop();

		deleteTex(&texMan, temp);
	}
}

//----------------------------------------
//
// Function: setTexture
//
// Loads (and initializes) textures to use with the polygon
//
// @pre     -
// @post    The texture is locked and loaded
// @param   format: The texture format (if we want to clamp or mirror it)
// @param   texpal:
// @return  -
//
//----------------------------------------


static void setTexture(unsigned int format, unsigned int texpal){

	textureFormat = format;
	texturePalette = texpal;

	TexCacheItem* newTexture = TexCache_SetTexture(TexFormat_32bpp, format, texpal);
	
	if(newTexture != currTexture){
		currTexture = newTexture;
		//Has the renderer initialized the texture already?
		if(!currTexture->deleteCallback){
			currTexture->deleteCallback = texDeleteCallback;
			if(freeTextureIds.empty()) expandFreeTextures();
			currTexture->texid = (u64)freeTextureIds.front();
			freeTextureIds.pop();

			activateTex(&texMan, (u32)currTexture->texid);

			// We must convert the texture into a GX-friendly format
			u8* src = currTexture->decoded;

			u32 curTexSizeY = currTexture->sizeY;
			u32 curTexSizeX = currTexture->sizeX;
			for(u32 y = 0; y < curTexSizeY; y++){
				for(u32 x = 0; x < curTexSizeX; x++){
					const u8 a = *src++;
					const u8 b = *src++;
					const u8 g = *src++;
					const u8 r = *src++;

				    const u32 offset = (((y >> 2)<<4)*currTexture->sizeX) + ((x >> 2)<<6) + (((y%4 << 2) + x%4 ) <<1);

					tmp_texture[offset]    = a;
					tmp_texture[offset+1]  = r;
					tmp_texture[offset+32] = g;
					tmp_texture[offset+33] = b;
				
				}
			}

			memcpy(currTexture->decoded,tmp_texture,currTexture->decode_len);
					
			// Make sure everything is finished before we move on.
			DCFlushRange(currTexture->decoded, currTexture->decode_len);
			
			// Put that data into a texture
			GX_InitTexObj(&gxtextures[(u32)currTexture->texid],
				currTexture->decoded, 
				currTexture->sizeX, 
				currTexture->sizeY, 
				GX_TF_RGBA8,
				(BIT16(currTexture->texformat) ? (BIT18(currTexture->texformat)?GX_MIRROR:GX_REPEAT) : GX_CLAMP), 
				(BIT17(currTexture->texformat) ? (BIT19(currTexture->texformat)?GX_MIRROR:GX_REPEAT) : GX_CLAMP), 
				GX_FALSE
			);

			// Now load it!
			GX_LoadTexObj(&gxtextures[(u32)currTexture->texid], GX_TEXMAP0);
			
		}else{
			// It's already been created, just load it
			GX_LoadTexObj(&gxtextures[(u32)currTexture->texid], GX_TEXMAP0);
		}

		// Configure the texture matrix 
		guMtxIdentity(textureView);
		guMtxScale(textureView, currTexture->invSizeX, currTexture->invSizeY, 1.0f);
		GX_LoadTexMtxImm(textureView,GX_TEXMTX0,GX_MTX3x4);
		GX_SetTexCoordGen(GX_TEXCOORD0,GX_TG_MTX3x4, GX_TG_TEX0, GX_TEXMTX0);
	
	}

}

//----------------------------------------
//
// Function: BeginRenderPoly
//
// Sets the variables specific to the polygon (texture, for example)
//
// @pre     -
// @post    The variables are set and the textures are loaded
// @param   -
// @return  -
//
//----------------------------------------

static void BeginRenderPoly(){
	bool enableDepthWrite = true;

	//GX_SetZMode(GX_ENABLE,depthFuncMode,GX_TRUE);

	if (cullingMask != 0xC0){
		GX_SetCullMode(map3d_cull[cullingMask>>6]);
	}
	else{
		GX_SetCullMode(GX_CULL_NONE);
	}

	// Initialize and load the texture. Make it so!
	setTexture(textureFormat, texturePalette);

	if(isTranslucent)
		enableDepthWrite = alphaDepthWrite;

#ifdef TODO
	//--DCN: GX has no stencil buffer! We'll have to find another way.

	//handle shadow polys
	if(envMode == 3){
		xglEnable(GL_STENCIL_TEST);
		if(polyID == 0) {
			enableDepthWrite = false;
			if(stencilStateSet!=0) {
				stencilStateSet = 0;
				//when the polyID is zero, we are writing the shadow mask.
                //set stencilbuf = 1 where the shadow volume is obstructed by geometry.
                //do not write color or depth information.

				// 2. The second parameter (65) is a reference value that we will test in glStensilOp, 
				// 3. The third parameter is a mask.
				// If a pixel should have been drawn to the screen, we want that spot marked with a 1. 
				// GL_ALWAYS does exactly that.
                glStencilFunc(GL_ALWAYS,65,255);

				// This tests for three different conditions based on the stencil function we decided to use.

				// 1. The first parameter tells OpenGL what to do if the test fails. 
				// Because the first parameter is GL_KEEP, if the test fails 
				// (which it can't because we have the funtion set to GL_ALWAYS), 
				// we would leave the stencil value set at whatever it currently is. 
				// 2. The second parameter tells OpenGL what do do if the stencil test passes, but the depth test fails. 
				// 3. The third parameter tells OpenGL what to do if the test passes! 
				// The value we put into the stencil buffer is our reference value ANDed with our mask value which is 255.
                glStencilOp(GL_KEEP,GL_REPLACE,GL_KEEP);

				// We don't want anything drawn to the screen at the moment, with all of the values set to 0 (GL_FALSE),
				// colors will not be drawn to the screen. 
                glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);

			}
		} else {
			enableDepthWrite = true;
			if(stencilStateSet!=1) {
				stencilStateSet = 1;
				//when the polyid is nonzero, we are drawing the shadow poly.
                //only draw the shadow poly where the stencilbuf==1.
                //I am not sure whether to update the depth buffer here--so I chose not to.

				// We're using GL_EQUAL to get where in the buffer the test passed (where it equals 1)
                glStencilFunc(GL_EQUAL,65,255);

				// As long as stencil testing is enabled pixels will ONLY be drawn if the stencil buffer has a value of 1.
				// If the stencil value is not 1 where the current pixel is being drawn it will not show up! GL_KEEP just 
				// tells OpenGL not to modify any values in the stencil buffer if the test passes OR fails! 
                glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);

				// We want to draw colors to the screen now.
                glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
             }
		}
	} else {
		xglEnable(GL_STENCIL_TEST);
		if(isTranslucent){
			stencilStateSet = 3;
			glStencilFunc(GL_NOTEQUAL,polyID,255);
			glStencilOp(GL_KEEP,GL_KEEP,GL_REPLACE);
			glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
		}
		else{
			if(stencilStateSet!=2) {
				stencilStateSet=2;
				glStencilFunc(GL_ALWAYS,64,255);
				glStencilOp(GL_REPLACE,GL_REPLACE,GL_REPLACE);
				glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
			}
		}
	}

#endif

	depthupdate = enableDepthWrite ? GX_TRUE : GX_FALSE;

}

//----------------------------------------
//
// Function: InstallPolygonAttrib
//
// Sets the variables specific to the polygon (texture, for example)
// Note: Remnant of OGLRender
//
// @pre     -
// @post    -
// @param   -
// @return  -
//
//----------------------------------------
static void InstallPolygonAttrib(unsigned long val){

	//--DCN: Culling is the only thing that we use right now,
	// But when I comment out all of them (except culling)
	// the colors change in SM64DS. WHY?
	//*
	// Light enable/disable
	lightMask = (val&0xF);

	// texture environment
	envMode = (val&0x30)>>4;

	// overwrite depth on alpha pass
	alphaDepthWrite = BIT11(val)!=0;

	// depth test function
	depthFuncMode = depthFunc[BIT14(val)];

	// back face culling
	cullingMask = (val&0xC0);

	alpha31 = ((val>>16)&0x1F)==31;

	// polyID
	polyID = (val>>24)&0x3F;
	//*/
}

//----------------------------------------
//
// Function: ReadFramebuffer
//
// Convert the rendered scene to a texture
//
// @pre     -
// @post    gfx3d_convertedScreen now contains the 3D scene
// @param   -
// @return  -
//
//----------------------------------------

static void ReadFramebuffer(){ 

	GX_DrawDone();

	GX_SetTexCopySrc(0, 0, 256, 192); 
	GX_SetTexCopyDst(256, 192, GX_TF_RGBA8, GX_FALSE);

	// Turn off vertical de-flicker filter temporary
    // (to avoid filtering during the framebuffer-to-texture copy)
	GX_SetCopyFilter(GX_FALSE, NULL, GX_FALSE, NULL);

	// Copy the screen into a texture
	GX_CopyTex((void*)GPU_screen3D, GX_TRUE);
	GX_PixModeSync();
	DCFlushRange(GPU_screen3D, 256*192*4);

	// Bleh, another "conversion" problem. In order to make our GX scene
	// jive with Desmume, we need to convert it OUT of its native format.
	u8* dst = gfx3d_convertedScreen;

	u8 *truc = (u8*)GPU_screen3D;
	u8 r, g, b, a;
    u32 offset;

	for(int y = 0; y < 192; y++){
		for(int x = 0; x < 256; x++){
	        
			offset = ((y >> 2)<< 12) + ((x >> 2)<<6) + ((((y%4) << 2) + (x%4)) << 1);

			a = *(truc+offset);
			r = *(truc+offset+1);
			g = *(truc+offset+32);
			b = *(truc+offset+33);

			*dst++ = (a >> 3) & 0x1F; // 5 bits
			*dst++ = (b >> 2) & 0x3F; // 6 bits
			*dst++ = (g >> 2) & 0x3F; // 6 bits
			*dst++ = (r >> 2) & 0x3F; // 6 bits

		}
	}

	DCFlushRange(gfx3d_convertedScreen, 256*192*4);

    // Restore vertical de-flicker filter mode
	GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter);

}

//----------------------------------------
//
// Function: GXRender
//
// Render the screen! Finally!
//
// @pre     -
// @post    The 3D has been rendered: no sprites
// @param   -
// @return  -
//
//----------------------------------------

static void GXRender(){

	// Lock our drawing thread
	LWP_MutexLock(vidmutex);

	// Set our video settings for 3D.
	Set3DVideoSettings();

	u32 lastTextureFormat = 0, lastTexturePalette = 0, lastPolyAttr = 0,
		polyListCount = gfx3d.polylist->count, lastViewport = 0xFFFFFFFF;

	for(u32 i = 0; i < polyListCount; ++i) {

		POLY *poly = &gfx3d.polylist->list[gfx3d.indexlist[i]];
		
		int type = poly->type;
		u8 alpha = (poly->getAlpha() << 3);	

		// If we have a new polygon texture...
		if( lastTextureFormat != poly->texParam || 
			lastTexturePalette != poly->texPalette || 
			lastPolyAttr != poly->polyAttr || i == 0 ){

			isTranslucent = poly->isTranslucent();
			InstallPolygonAttrib(lastPolyAttr = poly->polyAttr);
			lastTextureFormat = textureFormat = poly->texParam;
			lastTexturePalette = texturePalette = poly->texPalette;
			BeginRenderPoly();

		}

		//--DCN: I still don't see the point of this:
		//*
		if(lastViewport != poly->viewport){
			VIEWPORT viewport;
			viewport.decode(poly->viewport);
			GX_SetViewport((f32)viewport.x, (f32)viewport.y, (f32)viewport.width, (f32)viewport.height, 0.0f, 1.0f);
			lastViewport = poly->viewport;
		}
		//*/

		//GX_Begin((type == 3 ? GX_TRIANGLES : GX_QUADS), GX_VTXFMT0, type);
		GX_Begin(GX_TRIANGLEFAN, GX_VTXFMT0, type);

			int j = type - 1;
			do{
				VERT *vert = &gfx3d.vertlist->list[poly->vertIndexes[j]];

				// Have to flip the z coord
				GX_Position3f32(vert->x, vert->y, -vert->z);
				GX_Color4u8(vert->color[0], vert->color[1], vert->color[2], alpha);
				GX_TexCoord2f32(vert->u, vert->v);
				
				--j;
			}while(j >= 0);
		
		GX_End();

	}

	// Needs to happen before ending because it could free some textureids for expired cache items
	TexCache_EvictFrame();

	// Copy everything to a texture for later use
	ReadFramebuffer();

	// Reset everything back to what it was
	ResetVideoSettings();

	// Unlock the thread
	LWP_MutexUnlock(vidmutex);
		
}


//----------------------------------------
//
// Function: Set3DVideoSettings
//
// Sets the variables specific to our 3D scene
//
// @pre     -
// @post    -
// @param   -
// @return  -
//
//----------------------------------------

static void Set3DVideoSettings(){

	Mtx44 projection; // Projection matrix

	// Set up the viewpoint (one screen)
	GX_SetViewport(0,0,256,192,0,1);
	GX_SetScissor(0,0,256,192);

	//*
	// Our "not-quite" perspective projection. Needs tweaking/replacing.
	guPerspective(projection, 48.0f, 256.0f/192.0f, 1.0f, 1000.0f);
	GX_LoadProjectionMtx(projection, GX_PERSPECTIVE);
	//*/
	/*
	// No perspective projection at all; a different approach.
	guMtxIdentity(projection);
	GX_LoadProjectionMtx(projection, GX_PERSPECTIVE);
	GX_LoadPosMtxImm(projection,GX_PNMTX0)
	//*/

	//The only EFB pixel format supporting an alpha buffer is GX_PF_RGBA6_Z24
	GX_SetPixelFmt(GX_PF_RGBA6_Z24, GX_ZC_LINEAR);

	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);

	GX_InvVtxCache();
	GX_InvalidateTexAll();
	GX_ClearVtxDesc();

	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);

	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);


	if(gfx3d.enableTexturing){

		GX_SetNumTexGens(1);

		GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

		GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX3x4, GX_VA_TEX0, GX_IDENTITY); 

		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);

	}else{
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);			
	}

	GX_SetTevOp(GX_TEVSTAGE0, GX_REPLACE);

	if(gfx3d.enableAlphaTest){
	
		// OGLRender comment: FIXME: alpha test should pass gfx3d.alphaTestRef==poly->getAlpha
		
		// We need two comparisons, so we ignore the second parameter (for now)
		GX_SetAlphaCompare(GX_GREATER, gfx3d.alphaTestRef/31.f, GX_AOP_OR, GX_NEVER, 0);

	}else{		
		// We might be able to just ignore this instruction,
		// since we're not alpha blending.
		GX_SetAlphaCompare(GX_GREATER, 0, GX_AOP_OR , GX_NEVER, 0);
	}
	if(gfx3d.enableAlphaBlending){
		GX_SetAlphaUpdate(GX_TRUE);
	}else{
		GX_SetAlphaUpdate(GX_FALSE);
	}

	// In general, if alpha compare is enabled, Z-buffering 
	// should occur AFTER texture lookup.
	// This fixes the black-instead-of-alpha problem, but 
	// introduces its own problem: black is ALWAYS clear.
	//GX_SetZCompLoc(GX_FALSE);
}


//----------------------------------------
//
// Function: ResetVideoSettings
//
// Reset the video settings to what main.cpp needs
//
// @pre     -
// @post    The video settings are reset back to what they were
// @param   -
// @return  -
//
//----------------------------------------

static void ResetVideoSettings(){

	Mtx44 perspective;
	
	GX_SetViewport(0,0,rmode->fbWidth,rmode->efbHeight,0,1);
	GX_SetScissor(0,0,rmode->fbWidth,rmode->efbHeight);

	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

	GX_SetTevOp(GX_TEVSTAGE0, GX_REPLACE);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);	
	
	guOrtho(perspective,0,479,0,639,0,300);
	GX_LoadProjectionMtx(perspective, GX_ORTHOGRAPHIC);

	GX_SetCullMode(GX_CULL_NONE);
	GX_SetTexCoordGen(GX_TEXCOORD0,GX_TG_MTX3x4, GX_TG_TEX0, GX_IDENTITY);

	//GX_SetZCompLoc(GX_TRUE);
	//GX_SetZMode(GX_DISABLE, GX_EQUAL, GX_TRUE);
}

static void GXVramReconfigureSignal(){
	TexCache_Invalidate();
}

GPU3DInterface gpu3Dgx = {
	"GX",
	GXInit,
	GXReset,
	GXClose,
	GXRender,
	GXVramReconfigureSignal,
};
