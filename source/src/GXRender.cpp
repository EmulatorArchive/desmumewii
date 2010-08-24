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
    along with DeSmuMEWii; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <queue>
#include <gctypes.h>
#include <gccore.h>

#include "types.h"
#include "debug.h"
#include "GXRender.h"
#include "MMU.h"
#include "bits.h"
#include "matrix.h"
#include "NDSSystem.h"
#include "GXTexManager.h"
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

static const u8  map3d_cull[4] = {GX_CULL_ALL, GX_CULL_FRONT, GX_CULL_BACK, GX_CULL_NONE};
static const int texEnv[4] = { GX_MODULATE, GX_DECAL, GX_MODULATE, GX_MODULATE };
static const int depthFunc[2] = { GX_LESS, GX_EQUAL };

//Derived values extracted from polyattr etc
static u32 polyID = 0;
static u32 depthFuncMode = GX_LESS;
static u32 envMode = 0;
static u32 cullingMask = 0;
static u32 lightMask = 0;
static u32 textureFormat = 0, texturePalette = 0;
static bool alpha31 = false;
static bool alphaDepthWrite;
static bool isTranslucent;

static std::queue<u32> freeTextureIds;
static TexCacheItem* currTexture = NULL;

//------------------------------------------------------------
// Texture Variables 
//------------------------------------------------------------

// The number that we expand our texture array by
#define EXPAND_FREE_TEX_NUM 128

// Our texture manager (keeps track of which textures we're using)
TexManager* texMan;	

// When we need to apply a texture, we use this matrix
static Mtx textureView; 

//------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------
static void expandFreeTextures();
static void texDeleteCallback(TexCacheItem* item);

static void setTexture(u32 format, u32 texpal);
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
// Function: expandFreeTextures
//
// Increases the number of free texture IDs that we can use
//
// @pre     -
// @post    The available number of textures we can use has incresed
// @param   -
// @return  -
//
//----------------------------------------

static void expandFreeTextures(){

	u32 i = texMan->size();
	u32 newMax = i + EXPAND_FREE_TEX_NUM;
	for(; i < newMax; i++)
		freeTextureIds.push(i);

	// Resize our texture array to account for the new size
	if(!texMan->resize(newMax))
		printf("\n -- expandFreeTextures(): No more memory --\n");
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
	freeTextureIds.push(item->texid);
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

	texMan->reset();

	memset(GPU_screen3D, 0, sizeof(GPU_screen3D));
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

	// Create our Texture Manager
	texMan = new TexManager();

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
		//u32 temp = freeTextureIds.front();
		freeTextureIds.pop();	
	}
	// Kill our texture manager
	if(texMan)
		delete texMan;
	texMan = NULL;
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
// @param   texpal: The texture palette
// @return  -
//
//----------------------------------------

static void setTexture(u32 format, u32 texpal){

	textureFormat = format;
	texturePalette = texpal;

	TexCacheItem* newTexture = TexCache_SetTexture(TexFormat_32bpp, format, texpal);
	
	if(newTexture != currTexture){
		currTexture = newTexture;
		//Has the renderer initialized the texture already?
		if(!currTexture->deleteCallback){
			currTexture->deleteCallback = texDeleteCallback;
			if(freeTextureIds.empty()) expandFreeTextures();
			currTexture->texid = freeTextureIds.front();
			freeTextureIds.pop();

			texMan->activateTex(currTexture->texid);

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

				    const u32 offset = (((y >> 2)<<4)*curTexSizeX) + ((x >> 2)<<6) + (((y%4 << 2) + x%4 ) <<1);

					tmp_texture[offset]    = a;
					tmp_texture[offset+1]  = r;
					tmp_texture[offset+32] = g;
					tmp_texture[offset+33] = b;
				
				}
			}

			memcpy(currTexture->decoded, tmp_texture, currTexture->decode_len);
					
			// Make sure everything is finished before we move on.
			DCFlushRange(currTexture->decoded, currTexture->decode_len);
			
			// Put that data into a texture
			GX_InitTexObj(texMan->gxObj(currTexture->texid),
				currTexture->decoded, 
				curTexSizeX, 
				curTexSizeY, 
				GX_TF_RGBA8,
				(BIT16(currTexture->texformat) ? (BIT18(currTexture->texformat)?GX_MIRROR:GX_REPEAT) : GX_CLAMP), 
				(BIT17(currTexture->texformat) ? (BIT19(currTexture->texformat)?GX_MIRROR:GX_REPEAT) : GX_CLAMP), 
				GX_FALSE
			);
		}else{
			// It's already been created, continue.
		}
		GX_LoadTexObj(texMan->gxObj(currTexture->texid), GX_TEXMAP0);

		// Configure the texture matrix 
		guMtxIdentity(textureView);
		guMtxScale(textureView, currTexture->invSizeX, currTexture->invSizeY, 1.0f);
		GX_LoadTexMtxImm(textureView, GX_TEXMTX0,GX_MTX3x4);
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

	GX_SetZMode(GX_ENABLE, depthFuncMode, (enableDepthWrite ? GX_ENABLE : GX_DISABLE));
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

	//--DCN: Variables are not used (yet)
	/*
	// Light enable/disable
	lightMask = (val&0xF);

	// Texture environment
	envMode = (val&0x30)>>4;
	alpha31 = ((val>>16)&0x1F)==31;

	// Polygon ID (for shadows)
	polyID = (val>>24)&0x3F;
	//*/

	// Overwrite depth on alpha pass
	alphaDepthWrite = BIT11(val) != 0;

	// Depth test function
	depthFuncMode = depthFunc[BIT14(val)];

	// Back face culling
	cullingMask = (val & 0xC0);

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


#ifdef USE_CONVERTER
	GX_CopyTex(gfx3d_convertedScreen, GX_TRUE);
	GX_PixModeSync();
#else
	// Copy the screen into a texture
	GX_CopyTex((void*)GPU_screen3D, GX_TRUE);
	GX_PixModeSync();
	//--DCN: PixModeSync should take care of flushing.
	//DCFlushRange(GPU_screen3D, 256*192*4);
	// Bleh, another "conversion" problem. In order to make our GX scene
	// jive with Desmume, we need to convert it OUT of its native format.
	u8* dst = gfx3d_convertedScreen;

	u8 *truc = (u8*)GPU_screen3D;
	u8 r, g, b, a;
    u32 offset;

	for(u32 y = 0; y < 192; y++){
		for(u32 x = 0; x < 256; x++){
	        
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
#endif
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

#ifdef GX_3D_FUNCTIONS	
			if(poly->projMatrix[3][2] != 1){					
				GX_LoadProjectionMtx(poly->projMatrix, GX_PERSPECTIVE); 
			}else{
				GX_LoadProjectionMtx(poly->projMatrix, GX_ORTHOGRAPHIC); 
			}

			GX_LoadPosMtxImm(poly->mvMatrix, GX_PNMTX0);
#else
			// Create our own, DS-to-Wii specific projection matrix
			Mtx44 projection;

			float* m = poly->projMatrix;
			// Copy the matrix from Column-Major to Row-Major format
			for(int j = 0; j < 4; ++j)
				for(int i = 0; i < 4; ++i)
					projection[i][j] = *m++;
			
			// Convert the z clipping planes from -1/1 to -1/0
			projection[2][2] = 0.5*projection[2][2] - 0.5*projection[3][2];
			projection[2][3] = 0.5*projection[2][3] - 0.5*projection[3][3];

			if(projection[3][2] != 1) 
			{		
				//Frustum or perspective ?
				/* -- do we need this? just comment for now to remind me in future

				if(projection[0][2] != 0)
					//frustrum
				else
					//prespective
				//*/
					
				GX_LoadProjectionMtx(projection, GX_PERSPECTIVE); 
			}else{
				GX_LoadProjectionMtx(projection, GX_ORTHOGRAPHIC); 
			}
			/*
			Mtx modelview;
			guMtxIdentity(modelview);
			// Load in an identity matrix to be our position matrix
			GX_LoadPosMtxImm(modelview, GX_PNMTX0);
			//*/
#endif
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

		GX_Begin(GX_TRIANGLEFAN, GX_VTXFMT0, type);

			int j = type - 1;
			do{
				VERT *vert = &gfx3d.vertlist->list[poly->vertIndexes[j]];

				GX_Position3f32(vert->x, vert->y, vert->z);
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
	Mtx modelview;

	// Set up the viewpoint (one screen)
	GX_SetViewport(0,0,256,192,0,1);
	GX_SetScissor(0,0,256,192);

	guMtxIdentity(modelview);
	// Load in an identity matrix to be our position matrix
	GX_LoadPosMtxImm(modelview, GX_PNMTX0);
	/*
	// Our "not-quite" perspective projection. Needs tweaking/replacing.
	guPerspective(projection, 60.0f, 1, 1.0f, 1000.0f);
	GX_LoadProjectionMtx(projection, GX_PERSPECTIVE);
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
		GX_SetAlphaCompare(GX_GREATER, s32(s32(gfx3d.alphaTestRef)/31.f), GX_AOP_OR, GX_NEVER, 0);
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

	//--DCN: For reasons that I could not find, "Vanilla" does not
	// have any fog in its OpenGL implementation.

	if(gfx3d.enableFog && CommonSettings.GFX3D_Fog){

		//TODO: Make fogColor a GXColor so we won't have to convert it
		GXColor col = {
			GFX3D_5TO6((gfx3d.fogColor)&0x1F),
			GFX3D_5TO6((gfx3d.fogColor>>5)&0x1F),
			GFX3D_5TO6((gfx3d.fogColor>>10)&0x1F),
			(gfx3d.fogColor>>16)&0x1F
		};

		//--DCN: I just picked random numbers here.
		GX_SetFog(GX_FOG_LIN, 16.0f, 1000.0f, 0.0f, 1.0f, col);

		/*
		// There is no function to initialize the GXFogAdjTable.
		// If it DID exist, we would call it like so:
		GXFogAdjTbl table;
		//
		// Function: GX_InitFogAdjTable
		//
		// @param: GXFogAdjTable* table: The Fog adjustment table
		// @param: u16 width:     The width of our current viewport
		// @param: Mtx44 projmtx: The projection matrix that we're using
		GX_InitFogAdjTable(&table, 256, projection);
		// 
		// I believe that GX_SetFogRangeAdj does not do
		// what it is supposed to do, seeing as how none of 
		// the variables passed are used in the function.
		GX_SetFogRangeAdj(GX_ENABLE, 256/2 , &table);
		//*/
	}


	// In general, if alpha compare is enabled, Z-buffering 
	// should occur AFTER texture lookup.
	GX_SetZCompLoc(GX_FALSE);

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

	GX_SetZCompLoc(GX_TRUE);
	GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);

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
