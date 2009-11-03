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

#ifndef OGLRENDER_H
#define OGLRENDER_H

#include "common.h"
#include <algorithm>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "debug.h"
#include "render3D.h"
#include <gccore.h> //Main libogc header

#define DEFAULT_FIFO_SIZE (1024*1024)

#define GX_FRONT 1028
#define GX_BACK 1029
#define GX_FRONT_AND_BACK 1032
#define GX_VERTEX_SHADER 35633
#define GL_TRUE 1
#define GL_COMPILE_STATUS 35713
#define GL_INFO_LOG_LENGTH 35716
#define GL_FRAGMENT_SHADER 35632
#define GL_LINK_STATUS 35714
#define GL_TEXTURE_2D 3553
#define GL_TEXTURE 5890
#define GL_TEXTURE_MIN_FILTER 10241
#define GL_NEAREST 9728
#define GL_TEXTURE_MAG_FILTER 10240
#define GL_TEXTURE_WRAP_S 10242
#define GL_MIRRORED_REPEAT 33648
#define GL_REPEAT 10497
#define GL_CLAMP 10496
#define GL_TEXTURE_WRAP_T 10243
#define GL_RGBA 6408
#define GL_UNSIGNED_BYTE 5121
#define GL_PACK_ALIGNMENT 3333
#define GL_NORMALIZE 2977
#define GL_TEXTURE_1D 3552
#define GL_GREATER 516
#define GL_ALPHA_TEST 3008
#define GL_NO_ERROR 0
#define GL_SRC_ALPHA 770
#define GL_ONE_MINUS_SRC_ALPHA 771
#define GL_ONE 1
#define GL_DST_ALPHA 772
#define GL_TEXTURE1 33985
#define GL_TEXTURE0 33984
#define GL_CULL_FACE 2884
#define GL_FILL 6914
#define GL_LINE 6913
#define GL_STENCIL_TEST 2960
#define GL_ALWAYS 519
#define GL_KEEP 7680
#define GL_REPLACE 7681
#define GL_EQUAL 514
#define GL_NOTEQUAL 517
#define GL_TEXTURE_ENV 8960
#define GL_TEXTURE_ENV_MODE 8704
#define GL_BGRA 32993
#define GL_RGB 6407
#define GL_COLOR_BUFFER_BIT 16384
#define GL_DEPTH_BUFFER_BIT 256
#define GL_STENCIL_BUFFER_BIT 1024
#define GL_PROJECTION 5889

//Thanks profetlyn for lots of OGL conversion code to get the Wii version of DeSmuME to compile:

// OpenGl types
typedef u64 GLenum;
typedef u8 GLboolean;
typedef u64 GLuint;
typedef long int GLint;
typedef char GLchar;
typedef unsigned char GLubyte;


// Static variables
static bool depthTestEnabled=false;
static bool depthBufferWritingEnabled=true;

static GLenum currentDepthFunction=GX_NEVER;
static GLenum currentPolygonMode=GX_POINTS;

// Constants
GLenum GL_DEPTH_TEST=0;
GLenum GL_EXTENSIONS = 5;

extern GPU3DInterface gpu3Dgl;

//This is called by OGLRender whenever it initializes.
//Platforms, please be sure to set this up.
//return true if you successfully init.
extern bool (*oglrender_init)();

//This is called by OGLRender before it uses opengl.
//return true if youre OK with using opengl
extern bool (*oglrender_beginOpenGL)();

//This is called by OGLRender after it is done using opengl.
extern void (*oglrender_endOpenGL)();

#endif
