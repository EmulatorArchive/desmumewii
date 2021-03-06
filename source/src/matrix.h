/*  Copyright (C) 2006-2007 shash
	Copyright (C) 2009 DeSmuME team
    Copyright (C) 2012 DeSmuMEWii team

    This file is part of DeSmuMEWii

    DeSmuMEWii is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    DeSmuMEWii is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with DeSmuMEWii; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef MATRIX_H
#define MATRIX_H

#include <math.h>
#include <string.h>

#include "types.h"
#include "mem.h"

#define ENABLE_PAIRED_SINGLE

#ifdef ENABLE_PAIRED_SINGLE
#include "guDesmume.h"
#endif

struct MatrixStack
{
	MatrixStack(int size);
	float	*matrix;
	s32		position;
	s32		size;
};

void	MatrixInit					(float *matrix);
float	MatrixGetMultipliedIndex	(int index, float *matrix, float *rightMatrix);
void	MatrixSet					(float *matrix, int x, int y, float value);
int		MatrixCompare				(const float * matrixDST, const float * matrixSRC);
void	MatrixIdentity				(float *matrix);

void	MatrixTranspose				(float *matrix);
void	MatrixStackInit				(MatrixStack *stack);
void	MatrixStackSetMaxSize		(MatrixStack *stack, int size);
void	MatrixStackSetStackPosition (MatrixStack *stack, int pos);
void	MatrixStackPushMatrix		(MatrixStack *stack, const float *ptr);
float*	MatrixStackPopMatrix		(MatrixStack *stack, int size);
float*	MatrixStackGetPos			(MatrixStack *stack, int pos);
float*	MatrixStackGet				(MatrixStack *stack);
void	MatrixStackLoadMatrix		(MatrixStack *stack, int pos, const float *ptr);

void Vector2Copy(float *dst, const float *src);
void Vector2Add(float *dst, const float *src);
void Vector2Subtract(float *dst, const float *src);
float Vector2Dot(const float *a, const float *b);
float Vector2Cross(const float *a, const float *b);

float Vector3Dot(const float *a, const float *b);
void Vector3Cross(float* dst, const float *a, const float *b);
float Vector3Length(const float *a);
void Vector3Add(float *dst, const float *src);
void Vector3Subtract(float *dst, const float *src);
void Vector3Scale(float *dst, const float scale);
void Vector3Copy(float *dst, const float *src);
void Vector3Normalize(float *dst);

void Vector4Copy(float *dst, const float *src);

//these functions are an unreliable, inaccurate floor.
//it should only be used for positive numbers
//this isnt as fast as it could be if we used a visual c++ intrinsic, but those appear not to be universally available
FORCEINLINE u32 u32floor(float f)
{
	return (u32)f;
}
FORCEINLINE u32 u32floor(double d)
{
	return (u32)d;
}

//same as above but works for negative values too.
//be sure that the results are the same thing as floorf!
FORCEINLINE s32 s32floor(float f)
{
	return (s32)floorf(f);
}

template<int NUM>
static FORCEINLINE void memset_u16_le(void* dst, u16 val)
{
	for(int i=0;i<NUM;i++)
		T1WriteWord((u8*)dst,i<<1,val);
}

//---------------------------
//switched SSE functions

#ifdef ENABLE_PAIRED_SINGLE
#define MatrixMultVec4x4  ps_MatrixMultVec4x4
#define MatrixMultVec3x3  ps_MatrixMultVec3x3
#define MatrixMultiply    ps_MatrixMultiply
#define MatrixTranslate   ps_MatrixTranslate
#define MatrixScale       ps_MatrixScale
#define MatrixCopy        ps_MatrixCopy
#define guMtxDesmumeTrans ps_guMtxDesmumeTrans

#else // c versions

void MatrixMultVec4x4 (const float *matrix, float *vecPtr);
void MatrixMultVec3x3(const float * matrix, float * vecPtr);
void MatrixMultiply(float * matrix, const float * rightMatrix);
void MatrixTranslate(float *matrix, const float *ptr);
void MatrixScale(float * matrix, const float * ptr);
void MatrixCopy(float * matrixDST, const float * matrixSRC);

FORCEINLINE void mtx_fix2float4x4(float* matrix, const float divisor){
	for(int i=0;i<4*4;i++)
		matrix[i] /= divisor;
}

FORCEINLINE void mtx_fix2float3x4(float* matrix, const float divisor){
	for(int i=0;i<3*4;i++)
		matrix[i] /= divisor;
}

// Defines
#define guMtxDesmumeTrans c_guMtxDesmumeTrans

#endif //switched SSE functions

FORCEINLINE void MatrixMultVec4x4_M2(const float *matrix, float *vecPtr)
{
	//there are hardly any gains from merging these manually
	MatrixMultVec4x4(matrix+16,vecPtr);
	MatrixMultVec4x4(matrix,vecPtr);
}

template<int NUM_ROWS>
FORCEINLINE void vector_fix2float(float* matrix, const float divisor)
{
	for(int i=0;i<NUM_ROWS*4;i++)
		matrix[i] /= divisor;
}

template<int NUM>
static FORCEINLINE void memset_u8(void* dst, u8 val)
{
	memset(dst,val,NUM);
}




#endif
