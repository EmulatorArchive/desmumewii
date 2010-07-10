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

#ifndef GXRENDER_H
#define GXRENDER_H

#include <gctypes.h>
#include <gccore.h>
#include "common.h"
#include <algorithm>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "debug.h"
#include "render3D.h"

extern GPU3DInterface gpu3Dgx;

//This is called by GXRender whenever it initializes.
//return true if you successfully init.
extern bool (*gxrender_init)();

//This is called by GXRender before it uses GX.
//return true if you're OK with using GX
extern bool (*gxrender_beginGX)();

//This is called by GXRender after it is done.
extern void (*gxrender_endGX)();

#endif
