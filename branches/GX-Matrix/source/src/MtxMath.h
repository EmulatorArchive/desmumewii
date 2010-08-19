/*
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

#ifndef _matrixmath_h_
#define _matrixmath_h_

#include <ogcsys.h>
#include <gccore.h>


extern "C" {
void ps_guMtx44MultQuat(register Mtx44 m, register guQuaternion *a, register guQuaternion *d);
void ps_guMtxMultQuat(register Mtx m, register guQuaternion *a, register guQuaternion *d);
void ps_guMtx44MultVec(register Mtx44 mt, register guVector *src, register guVector *dst);


void ps_guMtx44MultVec(register Mtx44 mt, register guVector *src, register guVector *dst);
void ps_guMtx44Concat(const register Mtx44 a, const register Mtx44 b, register Mtx44 ab);
void ps_guMtx44Transpose(register Mtx44 src, register Mtx44 xPose);
void ps_guMtx44ApplyTrans(register Mtx44 src, register Mtx44 dst, register f32 xT, register f32 yT, register f32 zT);

void ps_guMtx44Scale(register Mtx44 mt,register f32 xS,register f32 yS,register f32 zS);

}

void c_guMtx44Identity(Mtx44 mt){
	s32 i,j;

	for(i=0;i<4;i++) {
		for(j=0;j<4;j++) {
			if(i==j) mt[i][j] = 1.0;
			else mt[i][j] = 0.0;
		}
	}
}

void c_guMtx44Copy(Mtx44 src,Mtx44 dst){
	if(src==dst) return;

    dst[0][0] = src[0][0]; dst[0][1] = src[0][1]; dst[0][2] = src[0][2]; dst[0][3] = src[0][3];
    dst[1][0] = src[1][0]; dst[1][1] = src[1][1]; dst[1][2] = src[1][2]; dst[1][3] = src[1][3]; 
    dst[2][0] = src[2][0]; dst[2][1] = src[2][1]; dst[2][2] = src[2][2]; dst[2][3] = src[2][3]; 
	dst[3][0] = src[3][0]; dst[3][1] = src[3][1]; dst[3][2] = src[3][2]; dst[3][3] = src[3][3]; 

}

#ifdef USE_C

#define guMtx44MultQuat c_guMtx44MultQuat
#define guMtxMultQuat   c_guMtxMultQuat

#else // Use PS

#define guMtx44MultQuat ps_guMtx44MultQuat
#define guMtx44MultVec ps_guMtx44MultVec
#define guMtxMultQuat   ps_guMtxMultQuat

#define guMtx44Concat ps_guMtx44Concat
#define guMtx44Transpose ps_guMtx44Transpose
#define guMtx44ApplyTrans ps_guMtx44ApplyTrans

#define guMtx44Scale ps_guMtx44Scale
//TODO:
#define guMtx44Identity c_guMtx44Identity
#define guMtx44Copy c_guMtx44Copy
#define guMtxInvXPose ps_guMtxInvXPose

#endif




//--DCN: My added fix-to-float function
// (To replace "vector_fix2float")
//////////////////////////////////////////////////
//////////////////////////////////////////////////
//////////////////////////////////////////////////

static void fix2floatGX(Mtx44 matrix, float d){
					  
	//--DCN: Make into paired-single!

	float divisor = 1.0f / d;  // 1/4096 = 0.000244140625
	for(int i = 0; i < 4; i++){
		for(int j = 0; j < 4; j++){
			matrix[i][j] *= divisor;
		}
	}
}

//////////////////////////////////////////////////
//////////////////////////////////////////////////
//////////////////////////////////////////////////


#endif
		