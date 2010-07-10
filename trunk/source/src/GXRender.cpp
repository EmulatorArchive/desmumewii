/*
	Copyright (C) 2006 yopyop
	Copyright (C) 2006-2007 shash

    This file is part of DeSmuME

    DeSmuME is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    DeSmuME is distributed in the hope that it will be useful,
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

extern Mtx44 perspective;
// ----------------------------------------------------------
// OGLRender declared variables (relic)
static CACHE_ALIGN u8 GPU_screen3D[256*192*4];

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

u32 oglToonTableTextureID;

//--DCN: We will end up eliminating "shaders" entirely at some point.
// Right now these sections are here for reference.

#ifdef WII_SHADERS

bool hasShaders = false;
u32 vertexShaderID;
u32 fragmentShaderID;
u32 shaderProgram;

static u32 hasTexLoc;
static u32 texBlendLoc;
static bool hasTexture = false;
static u32 lastEnvMode = 0;

#endif

static TexCacheItem* currTexture = NULL;

bool depthupdate;

//------------------------------------------------------------
// Texture Variables (Special thanks to the fine people at gl2gx!)

static Mtx textureView; // When we need to apply a texture, we use this matrix

#define MAX_MIP_LEVEL 10

//#define MAX_ARRAY 32 
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

	u32 glFormat;
	
	u8 min_filter;
	u8 base_level;
	u8 max_level;

	bool level[MAX_MIP_LEVEL+1];
	
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
	const s32 kInitTextures = 128;
	u32 gxTextureID[kInitTextures];

	//--DCN: You can try to push gxTextureID,
	//  I think this is just a remnant of OGL
	gxGenTextures(kInitTextures, &gxTextureID[0]);
	for(s32 i=0;i<kInitTextures;i++){
		freeTextureIds.push(i);
		//freeTextureIds.push(gxTextureID[i]);
	}
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
		//--DCN: Should we delete it?
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

#ifdef WII_SHADERS

	//GX:
	hasTexLoc = 0;
	hasTexture = false;
	texBlendLoc = 0;
		
#endif

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
	
#ifdef WII_SHADERS
	u32 loc = 0;
	/* Create the shaders */
	createShaders();

	/* Assign the texture units : 0 for main textures, 1 for toon table */
	/* Also init the locations for some variables in the shaders */

	loc = glGetUniformLocation(shaderProgram, "tex2d");
	glUniform1i(loc, 0);

	loc = glGetUniformLocation(shaderProgram, "toonTable");
	glUniform1i(loc, 1);

	hasTexLoc = glGetUniformLocation(shaderProgram, "hasTexture");

	texBlendLoc = glGetUniformLocation(shaderProgram, "texBlending");

	glGenTextures (1, &oglToonTableTextureID);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_1D, oglToonTableTextureID);
	glTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP); //clamp so that we don't run off the edges due to 1.0 -> [0,31] math
#endif

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

#ifdef WII_SHADERS
		glUseProgram(0);

		glDetachShader(shaderProgram, vertexShaderID);
		glDetachShader(shaderProgram, fragmentShaderID);

		glDeleteProgram(shaderProgram);
		glDeleteShader(vertexShaderID);
		glDeleteShader(fragmentShaderID);

		hasShaders = false;
#endif

	//kill the tex cache to free all the texture ids
	TexCache_Reset();

	while(!freeTextureIds.empty()){
		u32 temp = freeTextureIds.front();
		freeTextureIds.pop();

		deleteTex(&texMan, temp);
	}
	
#ifdef WII_SHADERS
	//Specific to WII_SHADERS
	deleteTex(&texMan, &oglToonTableTextureID);

#endif
	
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

#ifdef WII_SHADERS

	u32 textureMode = (unsigned short)((format>>26)&0x07);	
	
	//GX:
	if (format == 0 || textureMode == 0){
		if(hasShaders && hasTexture) { 
			hasTexLoc = 0; 
			hasTexture = false; 
		}
		return;
	}
	if(hasShaders){
		if(!hasTexture) { 
			hasTexLoc = 1; 
			hasTexture = true; 
		}
#ifdef TODO
		glActiveTexture(GL_TEXTURE0);
#endif
	}
#endif

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

#ifndef REVERSE_TEXTURE_SHIFTING

			// Here is the very basic "working" conversion
			u32 i = 0;
			for(u32 x = 0; x < currTexture->sizeX; x++){
				for(u32 y = 0; y < currTexture->sizeY; y++){
								
					const u32 t = i << 2;

					const u8 r = currTexture->decoded[t+3];
					const u8 g = currTexture->decoded[t+2];
					const u8 b = currTexture->decoded[t+1];
					const u8 a = currTexture->decoded[t+0];

					currTexture->decoded[t+1] = a;
					currTexture->decoded[t+0] = r;
					currTexture->decoded[t+3] = g;
					currTexture->decoded[t+2] = b;
				
					i++;
				}
			}
#else			
			//--DCN: Check it. Until we figure out how the
			// textures are actually stored (and re-write that to
			// jive with GX), let's convert the textures here.

#define RGB15_REVERSE(col) ( 0x8000 | (((col) & 0x001F) << 10) | ((col) & 0x03E0)  | (((col) & 0x7C00) >> 10) )
			// reverse engineered from Draw:
			u32 curX4 = currTexture->sizeX/4;
			u32 curY4 = currTexture->sizeY/4;

			u16* tempTex = new u16[currTexture->decode_len/2];
			u16* curTex = (u16*)currTexture->decoded;

			for (u32 y = 0; y < curY4; y++) {
				for (u32 h = 0; h < 4; h++) {
					for (u32 x = 0; x < curX4; x++) {
						for (int w = 0; w < 2; w++) {
							*tempTex++ = (curTex[w]);
						}
						const u8 a = curTex[0];
						const u8 r = curTex[1];
						const u8 g = curTex[2];
						const u8 b = curTex[3];
					
						*tempTex++ = a;
						*tempTex++ = r;
						*tempTex++ = g;
						*tempTex++ = b;
						
						tempTex += 12;
						curTex += 4;
					}

					
					tempTex -= 4*curX4 - 4;
				}
				tempTex += 4*curX4 - 4*4;
			}
			// Copy it back to the original
			memcpy(currTexture->decoded, tempTex, currTexture->decode_len* sizeof(u8));
			
#endif			
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
			// Otherwise, just bind it
			//--DCN: I think that this is unnecessary!
			//activateTex(&texMan, (u32)currTexture->texid);
			// And load it
			GX_LoadTexObj(&gxtextures[(u32)currTexture->texid], GX_TEXMAP0);
			
		}

		// Configure the texture matrix 
		guMtxIdentity(textureView);
		guMtxScale(textureView, currTexture->invSizeX, currTexture->invSizeY, 1.0f);
	
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

	if (cullingMask != 0xC0){
		//--DCN: I'm just setting it to this so it will DEFINITELY show something
		// I believe the map3d_cull array is incorrect
		GX_SetCullMode(GX_CULL_NONE);
		//GX_SetCullMode(map3d_cull[cullingMask>>6]);
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


#ifdef WII_SHADERS
		if(envMode != lastEnvMode){
			lastEnvMode = envMode;

			int _envModes[4] = {0, 1, (2 + gfx3d.shading), 0};

			//GX:
			texBlendLoc = _envModes[envMode];
			//OGL:
			//glUniform1i(texBlendLoc, _envModes[envMode]);
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
}

//----------------------------------------
//
// Function: Set3DVideoSettings (formerly "Control")
//
// Sets the variables specific to the polygon (texture, for example)
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

	guPerspective(projection, 45, 256/192, 1, 1000);
	GX_LoadProjectionMtx(projection, GX_PERSPECTIVE);
	
	
	//--DCN: Experimenting:
	//*
	GX_SetPixelFmt(GX_PF_RGBA6_Z24, GX_ZC_LINEAR);
	GX_SetZCompLoc(GX_TRUE);
	
	//Perhaps altering these variables would result in something
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);

	GX_InvVtxCache();
	GX_InvalidateTexAll();
	//*/

	
	GX_ClearVtxDesc();

	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);

	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);


	if(gfx3d.enableTexturing){

		GX_SetNumTexGens(1);

		GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX3x4, GX_VA_TEX0, GX_IDENTITY); 
		//GX_SetTexCoordGen(GX_VA_TEX0, GX_TG_MTX2x4, GX_TEXCOORD0, GX_IDENTITY);
		
		GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);

	}else{
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);			
	}
	
	GX_SetTevOp(GX_TEVSTAGE0, GX_REPLACE);

	if(gfx3d.enableAlphaTest){
	
		// OGLRender comment: FIXME: alpha test should pass gfx3d.alphaTestRef==poly->getAlpha
	
		// This is a hack (sorta), we need two comparisons! What's up with that?
		GX_SetAlphaCompare(GX_GREATER, gfx3d.alphaTestRef/31.f, GX_AOP_OR , GX_LESS, 0);

	}else{		
		// Also a hack: Just doing the same thing twice
		GX_SetAlphaCompare(GX_GREATER, 0, GX_AOP_AND , GX_GREATER, 0);
	}
	if(gfx3d.enableAlphaBlending){
		GX_SetAlphaUpdate(GX_TRUE);
	}else{
		GX_SetAlphaUpdate(GX_FALSE);
	}

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

	// Bleh, another "conversion" problem. In order to make our GX scene
	// jive with Desmume, we need to convert it OUT of its native format.
	
#ifndef NEED_3D_SCREEN_CONVERSION	
	// If we ever get to the point where we don't need to convert the colors,
	// THIS is how simple (and fast!) our solution will be. It's good to dream.
	GX_CopyTex(gfx3d_convertedScreen, GX_FALSE);
	GX_PixModeSync();
#else

	GX_CopyTex((void*)GPU_screen3D, GX_FALSE);
	GX_PixModeSync();

	u8* dst = gfx3d_convertedScreen;

	
	//*
	// "Working" version:

	for(int i = 0, y = 0; y < 192; y++){
		for(int x = 0; x < 256; x++, i++){
		
			const int t = i << 2;
			const u8 a = GPU_screen3D[t+1];
			const u8 r = GPU_screen3D[t+0];
			const u8 g = GPU_screen3D[t+3];
			const u8 b = GPU_screen3D[t+2];

			*dst++ = r;
			*dst++ = g;
			*dst++ = b;
			*dst++ = a;
	
		}
	}
	//*/

	/*

	// My hack (BLEH!) to "unconvert" the rendered data OUT 
	// of native format, because we "convert" it again in main
	u16 *sTop = (u16*)&GPU_screen3D;
	u16 *dTop = (u16*)gfx3d_convertedScreen;	
	
	//TODO: Make this the OPPOSITE of main.cpp, this is almost verbatim:
	for (int y = 0; y < 48; y++) {
		for (int h = 0; h < 4; h++) {
			for (int x = 0; x < 64; x++) {
				for (int w = 0; w < 4; w++) {
					*dTop++ = RGB15_REVERSE(sTop[w]);
				}
				dTop+=12;     // next tile
				sTop+=4;
			}
			dTop-=1020;     // next line
		}
		dTop+=1008;       // next row
	}
	//*/


#endif

	DCFlushRange(gfx3d_convertedScreen, 256*192*4);
	
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

	Mtx modelView;    // Model view matrix
	Mtx modelTex;     // Model texture matrix

	// Lock our drawing thread
	LWP_MutexLock(vidmutex);

#ifdef WII_SHADERS
		//TODO - maybe this should only happen if the toon table is stale (for a slight speedup)

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_1D, oglToonTableTextureID);
		
		//generate a 8888 toon table from the ds format one and store it in a texture
		u32 rgbToonTable[32];
		for(int i=0;i<32;i++) rgbToonTable[i] = RGB15TO32(gfx3d.u16ToonTable[i], 255);
		glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbToonTable);
#endif

	// Set our video settings for 3D.
	Set3DVideoSettings();

	guMtxIdentity(modelView);
	GX_LoadPosMtxImm(modelView, GX_PNMTX0);

	/*
	// Reserved for normals, usually:
	guMtxInverse(modelView, modelTex);
	guMtxTranspose(modelTex, modelTex);
	guMtxConcat(modelTex, textureView, modelTex);
	//*/
	guMtxConcat(modelView, textureView, modelTex);
	GX_LoadTexMtxImm(modelTex, GX_TEXMTX0, GX_MTX2x4);
	
	
	
	u32 lastTextureFormat = 0, lastTexturePalette = 0, 
		lastPolyAttr = 0, polyListCount = gfx3d.polylist->count;
	
	for(u32 i = 0; i < polyListCount; ++i) {
	
		POLY *poly = &gfx3d.polylist->list[gfx3d.indexlist[i]];
		
		// If we have a new polygon...
		if( lastTextureFormat != poly->texParam || 
			lastTexturePalette != poly->texPalette || 
			lastPolyAttr != poly->polyAttr || i == 0 ){

			isTranslucent = poly->isTranslucent();
			InstallPolygonAttrib(lastPolyAttr = poly->polyAttr);
			lastTextureFormat = textureFormat = poly->texParam;
			lastTexturePalette = texturePalette = poly->texPalette;
			BeginRenderPoly();

		}
		
		int type = poly->type;
		u8 alphaU8 = poly->getAlpha();

		GX_Begin((type == 3 ? GX_TRIANGLES : GX_QUADS), GX_VTXFMT0, type);

			int j = type - 1;
			do{

				VERT *vert = &gfx3d.vertlist->list[poly->vertIndexes[j]];

				// Have to flip the z coord
				GX_Position3f32(vert->x, vert->y, -vert->z);			
				GX_Color4u8(vert->color[0], vert->color[1], vert->color[2], alphaU8);
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
// Function: ResetVideoSettings
//
// Reset the video settings to what main.cpp needs
//
// @pre     -
// @post    -
// @param   -
// @return  -
//
//----------------------------------------

static void ResetVideoSettings(){
		
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
