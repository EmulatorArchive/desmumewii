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

// We must check if they are already defined in libogc.
// If they are, use the official functions. If not, use our own.

#ifndef guMtx44Identity
	void ps_guMtx44Identity(register Mtx44 mt);
	#define guMtx44Identity ps_guMtx44Identity
#endif

#ifndef guMtx44Copy
	void ps_guMtx44Copy(register Mtx44 src, register Mtx44 dst);
	#define guMtx44Copy ps_guMtx44Copy
#endif

#ifndef guMtx44Concat
	void ps_guMtx44Concat(const register Mtx44 a, const register Mtx44 b, register Mtx44 ab);
	#define guMtx44Concat ps_guMtx44Concat
#endif

#ifndef guMtx44Transpose
	void ps_guMtx44Transpose(register Mtx44 src, register Mtx44 xPose);
	#define guMtx44Transpose ps_guMtx44Transpose
#endif

#ifndef guMtx44Scale
	void ps_guMtx44Scale(register Mtx44 mt,register f32 xS,register f32 yS,register f32 zS);
	#define guMtx44Scale ps_guMtx44Scale
#endif

#ifndef guMtxMultQuat
	void ps_guMtxMultQuat(register Mtx m, register guQuaternion *a, register guQuaternion *d);
	#define guMtxMultQuat ps_guMtxMultQuat
#endif

#ifndef guMtx44MultQuat
	void ps_guMtx44MultQuat(register Mtx44 mt, register guQuaternion *src,register guQuaternion *dst);
	#define guMtx44MultQuat ps_guMtx44MultQuat
#endif

#ifndef guMtxInvXPose

	u32 ps_guMtxInvXpose_copyFromLibogc(register Mtx src, register Mtx xPose);
	//This should already be defined in libogc, actually.
	#define guMtxInvXPose ps_guMtxInvXpose_copyFromLibogc
#endif

}



void c_guMtx44MultVec ( const Mtx44 m, const guVector *src, guVector *dst ){
    guVector vTmp;
    f32 w;
    // a Vec has a 4th implicit 'w' coordinate of 1
    vTmp.x = m[0][0]*src->x + m[0][1]*src->y + m[0][2]*src->z + m[0][3];
    vTmp.y = m[1][0]*src->x + m[1][1]*src->y + m[1][2]*src->z + m[1][3];
    vTmp.z = m[2][0]*src->x + m[2][1]*src->y + m[2][2]*src->z + m[2][3];
    w      = m[3][0]*src->x + m[3][1]*src->y + m[3][2]*src->z + m[3][3];
    w = 1.0f/w;

    // copy back
    dst->x = vTmp.x * w;
    dst->y = vTmp.y * w;
    dst->z = vTmp.z * w;
}


void c_guMtx44ApplyTrans(Mtx44 src, Mtx44 dst, const f32 xT, const f32 yT, const f32 zT){
	if ( src != dst ){
		dst[0][0] = src[0][0];    dst[0][1] = src[0][1];    dst[0][2] = src[0][2];
		dst[1][0] = src[1][0];    dst[1][1] = src[1][1];    dst[1][2] = src[1][2];
		dst[2][0] = src[2][0];    dst[2][1] = src[2][1];    dst[2][2] = src[2][2];
		dst[3][0] = src[3][0];    dst[3][1] = src[3][1];    dst[3][2] = src[3][2];
	}

	dst[0][3] = src[0][0]*xT + src[0][1]*yT + src[0][2]*zT + src[0][3];
	dst[1][3] = src[1][0]*xT + src[1][1]*yT + src[1][2]*zT + src[1][3];
	dst[2][3] = src[2][0]*xT + src[2][1]*yT + src[2][2]*zT + src[2][3];
	dst[3][3] = src[3][0]*xT + src[3][1]*yT + src[3][2]*zT + src[3][3];
}





// These are unfinished
//TODO

#ifndef guMtx44ApplyTrans
	/*
	// This one is actually slightly slower!!
	void ps_guMtx44ApplyTrans(register Mtx44 src, register Mtx44 dst, register f32 xT, register f32 yT, register f32 zT);
	// Darn it, intertwining the instructions doesn't speed it up!
	#define guMtx44ApplyTrans ps_guMtx44ApplyTrans	
	//*/
	#define guMtx44ApplyTrans c_guMtx44ApplyTrans
#endif

#ifndef guMtx44MultVec
	// ps_guMtx44MultVec is a copy of ps_guMtx44MultQuat
	/*
	void ps_guMtx44MultVec(register Mtx44 mt, register guVector *src, register guVector *dst);
	#define guMtx44MultVec ps_guMtx44MultVec
	//*/
	#define guMtx44MultVec c_guMtx44MultVec
#endif




//--DCN: My added fix-to-float function
// (To replace "vector_fix2float")
//////////////////////////////////////////////////
//////////////////////////////////////////////////
//////////////////////////////////////////////////

static void fix2floatGX(Mtx44 matrix, const float d){
					  
	//--DCN: Make into paired-single!

	const float divisor = 1.0f / d;  // 1/4096 = 0.000244140625
	for(int i = 0; i < 4; i++){
		for(int j = 0; j < 4; j++){
			matrix[i][j] *= divisor;
		}
	}
}


#endif
		