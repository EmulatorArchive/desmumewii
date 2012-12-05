#ifndef __GUDESMUME_H__
#define __GUDESMUME_H__

#include <gctypes.h>


#ifdef ENABLE_PAIRED_SINGLE

extern "C" {
	void ps_MatrixMultVec4x4(register const float *matrix, register float *vecPtr);
	void ps_MatrixMultVec3x3(register float *matrix, register float *vecPtr);
	void ps_MatrixCopy(register float* matrixDST, register const float* matrixSRC);
	void ps_MatrixTranslate(register float *matrix, register float *ptr);
	void ps_MatrixScale(register float *matrix, register float *ptr);
	void ps_MatrixMultiply(register float* matrix, register float* rightMatrix);
	void ps_guMtxDesmumeTrans(register f32* outST, register f32* mtxCurrent,
							  register f32* coord, register f32* inST);
	void ps_mtx_fix2float4x4(register f32* matrix, register const f32 divisor);
	void ps_mtx_fix2float3x4(register f32* matrix, register const f32 divisor);
}

#else //No paired single

void c_guMtxDesmumeTrans(f32* outST, f32* mtxCurrent, f32* coord, f32* inST){
	outST[0] =((coord[0]*mtxCurrent[0] +
				coord[1]*mtxCurrent[4] +
				coord[2]*mtxCurrent[8]) + inST[0] * 16.0f) / 16.0f;
	outST[1] =((coord[0]*mtxCurrent[1] +
				coord[1]*mtxCurrent[5] +
				coord[2]*mtxCurrent[9]) + inST[1] * 16.0f) / 16.0f;
			
}

#endif


#endif