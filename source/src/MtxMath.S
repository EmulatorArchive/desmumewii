// Condition Register Bit Fields

#define	cr0	0
#define	cr1	1
#define	cr2	2
#define	cr3	3
#define	cr4	4
#define	cr5	5
#define	cr6	6
#define	cr7	7

// General Purpose Registers (GPRs)

#define	r0	0
#define	r1	1
#define	sp	1
#define	r2	2
#define	toc	2
#define	r3	3
#define	r4	4
#define	r5	5
#define	r6	6
#define	r7	7
#define	r8	8
#define	r9	9
#define	r10	10
#define	r11	11
#define	r12	12
#define	r13	13
#define	r14	14
#define	r15	15
#define	r16	16
#define	r17	17
#define	r18	18
#define	r19	19
#define	r20	20
#define	r21	21
#define	r22	22
#define	r23	23
#define	r24	24
#define	r25	25
#define	r26	26
#define	r27	27
#define	r28	28
#define	r29	29
#define	r30	30
#define	r31	31

// Floating Point Registers (FPRs)

#define	fr0		0
#define	fr1		1
#define	fr2		2
#define	fr3		3
#define	fr4		4
#define	fr5		5
#define	fr6		6
#define	fr7		7
#define	fr8		8
#define	fr9		9
#define	fr10	10
#define	fr11	11
#define	fr12	12
#define	fr13	13
#define	fr14	14
#define	fr15	15
#define	fr16	16
#define	fr17	17
#define	fr18	18
#define	fr19	19
#define	fr20	20
#define	fr21	21
#define	fr22	22
#define	fr23	23
#define	fr24	24
#define	fr25	25
#define	fr26	26
#define	fr27	27
#define	fr28	28
#define	fr29	29
#define	fr30	30
#define	fr31	31

#define	vr0		0
#define	vr1		1
#define	vr2		2
#define	vr3		3
#define	vr4		4
#define	vr5		5
#define	vr6		6
#define	vr7		7
#define	vr8		8
#define	vr9		9
#define	vr10	10
#define	vr11	11
#define	vr12	12
#define	vr13	13
#define	vr14	14
#define	vr15	15
#define	vr16	16
#define	vr17	17
#define	vr18	18
#define	vr19	19
#define	vr20	20
#define	vr21	21
#define	vr22	22
#define	vr23	23
#define	vr24	24
#define	vr25	25
#define	vr26	26
#define	vr27	27
#define	vr28	28
#define	vr29	29
#define	vr30	30
#define	vr31	31

//////////////////

	.globl ps_guMtx44Identity
	//r3 == mtx44
ps_guMtx44Identity:
	lis			r9,Unit01@ha
	addi		r9,r9,Unit01@l
	lfs			fr0,0(r9)
	lfs			fr1,4(r9)
	psq_st		fr0,8(r3),0,0
	ps_merge01	fr2,fr0,fr1
	psq_st		fr0,24(r3),0,0
	ps_merge10	fr3,fr1,fr0
	psq_st		fr0,32(r3),0,0
	psq_st		fr2,16(r3),0,0
	psq_st		fr3,0(r3),0,0
	psq_st		fr3,40(r3),0,0
	psq_st		fr0,48(r3),0,0
	psq_st		fr2,56(r3),0,0
	blr
	
	.globl ps_guMtx44Copy
	//r3 = src, r4 = dst
ps_guMtx44Copy:
	psq_l		fr0,0(r3),0,0
	psq_st		fr0,0(r4),0,0
	psq_l		fr1,8(r3),0,0
	psq_st		fr1,8(r4),0,0
	psq_l		fr2,16(r3),0,0
	psq_st		fr2,16(r4),0,0
	psq_l		fr3,24(r3),0,0
	psq_st		fr3,24(r4),0,0
	psq_l		fr4,32(r3),0,0
	psq_st		fr4,32(r4),0,0
	psq_l		fr5,40(r3),0,0
	psq_st		fr5,40(r4),0,0
	psq_l		fr6,48(r3),0,0
	psq_st		fr6,48(r4),0,0
	psq_l		fr7,56(r3),0,0
	psq_st		fr7,56(r4),0,0
	blr
	
	.globl ps_guMtxMultVecArray
ps_guMtxMultVecArray:
	psq_l       fr13,  0(r3), 0, 0
	psq_l       fr12, 16(r3), 0, 0
	subi        r6, r6, 1
	psq_l       fr11,  8(r3), 0, 0
	ps_merge00  fr0, fr13, fr12
	subi        r5, r5, 4
	psq_l       fr10, 24(r3), 0, 0
	ps_merge11  fr1, fr13, fr12
	mtctr       r6
	psq_l       fr4,  32(r3), 0, 0
	ps_merge00  fr2, fr11, fr10
	psq_l       fr5,  40(r3), 0, 0
	ps_merge11  fr3, fr11, fr10
	psq_l       fr6,  0(r4), 0, 0
	psq_lu      fr7,  8(r4), 1, 0
	ps_madds0   fr8, fr0, fr6, fr3
	ps_mul      fr9, fr4, fr6
	ps_madds1   fr8, fr1, fr6, fr8
	ps_madd     fr10, fr5, fr7, fr9
_mloop:
	psq_lu      fr6,  4(r4), 0, 0
	ps_madds0   fr12, fr2, fr7, fr8
	psq_lu      fr7,  8(r4), 1, 0
	ps_sum0     fr13, fr10, fr9, fr10
	ps_madds0   fr8, fr0, fr6, fr3
	ps_mul      fr9, fr4, fr6
	psq_stu     fr12,  4(r5), 0, 0
	ps_madds1   fr8, fr1, fr6, fr8
	psq_stu     fr13,  8(r5), 1, 0
	ps_madd     fr10, fr5, fr7, fr9
	bdnz        _mloop
	ps_madds0   fr12, fr2, fr7, fr8
	ps_sum0     fr13, fr10, fr9, fr10
	psq_stu     fr12,  4(r5), 0, 0
	psq_stu     fr13,  8(r5), 1, 0
	blr

	.globl ps_guMtx44MultVecArray
ps_guMtx44MultVecArray:
	stwu        r1, -16(r1)
	addi        r6, r6, -1
	psq_l       fr6, 48(r3), 0, 0
	mtctr       r6
	psq_l       fr8, 0(r4), 0, 0
	addi        r5, r5, -4
	psq_l       fr7, 56(r3), 0, 0
	psq_lu      fr9, 8(r4), 1, 0 
	ps_mul      fr13, fr6, fr8
	psq_l       fr0, 0(r3), 0, 0 
	stfd        fr14, 8(r1)
	ps_madd     fr13, fr7, fr9, fr13
	psq_l       fr2, 16(r3), 0, 0
	ps_merge11  fr14, fr9, fr9
	ps_mul      fr10, fr0, fr8
	psq_l       fr4, 32(r3), 0, 0 
	ps_mul      fr11, fr2, fr8
	psq_l       fr1, 8(r3), 0, 0 
	ps_mul      fr12, fr4, fr8 
	psq_l       fr3, 24(r3), 0, 0  
	ps_sum0     fr13, fr13, fr13, fr13
	psq_l       fr5, 40(r3), 0, 0

_m44loop:
	ps_madd     fr10, fr1, fr9, fr10
	ps_madd     fr11, fr3, fr9, fr11
	ps_madd     fr12, fr5, fr9, fr12
	ps_sum0     fr10, fr10, fr10, fr10
	ps_sum0     fr11, fr11, fr11, fr11
	ps_sum0     fr12, fr12, fr12, fr12
	ps_div      fr13, fr14, fr13
	psq_lu      fr8, 4(r4), 0, 0
	psq_lu      fr9, 8(r4), 1, 0
	ps_mul      fr10, fr10, fr13
	psq_stu     fr10, 4(r5), 1, 0
	ps_mul      fr11, fr11, fr13
	psq_stu     fr11, 4(r5), 1, 0
	ps_mul      fr12, fr12, fr13
	psq_stu     fr12, 4(r5), 1, 0
	ps_mul      fr13, fr6, fr8
	ps_mul      fr10, fr0, fr8
	ps_mul      fr11, fr2, fr8
	ps_madd     fr13, fr7, fr9, fr13
	ps_mul      fr12, fr4, fr8
	ps_sum0     fr13, fr13, fr13, fr13
	bdnz+       _m44loop
	ps_madd     fr10, fr1, fr9, fr10
	ps_madd     fr11, fr3, fr9, fr11
	ps_madd     fr12, fr5, fr9, fr12
	ps_sum0     fr10, fr10, fr10, fr10
	ps_sum0     fr11, fr11, fr11, fr11 
	ps_sum0     fr12, fr12, fr12, fr12
	ps_div      fr13, fr14, fr13
	ps_mul      fr10, fr10, fr13
	psq_st      fr10, 4(r5), 1, 0
	ps_mul      fr11, fr11, fr13
	psq_st      fr11, 8(r5), 1, 0
	ps_mul      fr12, fr12, fr13
	psq_st      fr12, 12(r5), 1, 0
	lfd			fr14,  8(r1)
	addi		r1, r1, 16
	blr
	
	
	.globl ps_guMtx44MultQuat
	//r3 = Mtx44, r4 = Quat, r5 = dstQuat
ps_guMtx44MultQuat:
	psq_l		fr0,0(r4),0,0
	psq_l		fr2,0(r3),0,0
	psq_l		fr1,8(r4),0,0
	ps_mul		fr4,fr2,fr0
	psq_l		fr3,8(r3),0,0
	ps_madd		fr5,fr3,fr1,fr4
	psq_l		fr8,16(r3),0,0
	ps_sum0		fr6,fr5,fr6,fr5
	psq_l		fr9,24(r3),0,0
	ps_mul		fr10,fr8,fr0
	psq_st		fr6,0(r5),1,0
	ps_madd		fr11,fr9,fr1,fr10
	psq_l		fr2,32(r3),0,0
	ps_sum0		fr12,fr11,fr12,fr11
	psq_l		fr3,40(r3),0,0
	ps_mul		fr4,fr2,fr0
	psq_st		fr12,4(r5),1,0
	ps_madd		fr5,fr3,fr1,fr4
	psq_l		fr8, 48(r3), 0, 0
	ps_sum0		fr6,fr5,fr6,fr5
	psq_l		fr9, 56(r3), 0, 0
	ps_mul		fr10, fr8, fr0
	psq_st		fr6,8(r5),1,0
	ps_madd		fr11, fr9, fr1, fr10
	ps_sum0		fr12, fr11, fr12, fr11
	psq_st		fr12, 12(r5), 1, 0
	blr
	
	////////////////////////////////////
	
	.globl ps_guMtxMultQuat
	//r3 = Mtx, r4 = quat, r5 = dstQuat
ps_guMtxMultQuat:
	psq_l		fr0,0(r4),0,0
	psq_l		fr2,0(r3),0,0
	psq_l		fr1,8(r4),1,0
	ps_mul		fr4,fr2,fr0
	psq_l		fr3,8(r3),0,0
	ps_madd		fr5,fr3,fr1,fr4
	psq_l		fr8,16(r3),0,0
	ps_sum0		fr6,fr5,fr6,fr5
	psq_l		fr9,24(r3),0,0
	ps_mul		fr10,fr8,fr0
	psq_st		fr6,0(r5),1,0
	ps_madd		fr11,fr9,fr1,fr10
	psq_l		fr2,32(r3),0,0
	ps_sum0		fr12,fr11,fr12,fr11
	psq_l		fr3,40(r3),0,0
	ps_mul		fr4,fr2,fr0
	psq_st		fr12,4(r5),1,0
	ps_madd		fr5,fr3,fr1,fr4
	psq_l		fr8, 48(r3), 0, 0
	ps_sum0		fr6,fr5,fr6,fr5
	psq_l		fr9, 56(r3), 0, 0
	ps_mul		fr10, fr8, fr0
	psq_st		fr6,8(r5),1,0
	ps_madd		fr11, fr9, fr1, fr10
	ps_sum0		fr12, fr11, fr12, fr11
	psq_st		fr12, 12(r5), 1, 0
	blr
	
	////////////////////////////////////
		
	.globl ps_guMtx44Transpose
	//r3 = src, r4 = xpose
ps_guMtx44Transpose:
	psq_l		fr0,0(r3),0,0
	psq_l		fr1,16(r3),0,0	
	ps_merge00	fr4,fr0,fr1
	psq_l		fr2,8(r3),0,0
	psq_st		fr4,0(r4),0,0
	ps_merge11	fr5,fr0,fr1
	psq_l		fr3,24(r3),0,0		
	psq_st		fr5,16(r4),0,0
	ps_merge00	fr4,fr2,fr3
	psq_l		fr0,32(r3),0,0
	psq_st		fr4,32(r4),0,0
	ps_merge11	fr5,fr2,fr3	
	psq_l		fr1,48(r3),0,0
	psq_st		fr5,48(r4),0,0
	ps_merge00	fr4,fr0,fr1
	psq_l		fr2,40(r3),0,0
	psq_st		fr4,8(r4),0,0
	ps_merge11	fr5,fr0,fr1
	//*
	// Is this as fast as it could be?
	// The last 4 instructions are dependent on each other	
	psq_l		fr3,56(r3),0,0
	psq_st		fr5,24(r4),0,0	
	ps_merge00	fr4,fr2,fr3
	psq_st		fr4,40(r4),0,0	
	ps_merge11	fr5,fr2,fr3		
	psq_st		fr5,56(r4),0,0
	//*/
	/*
	// Would the end be better this way? 
	psq_l		fr3,56(r3),0,0
	ps_merge00	fr4,fr2,fr3
	psq_st		fr5,24(r4),0,0	
	ps_merge11	fr5,fr2,fr3
	psq_st		fr4,40(r4),0,0		
	psq_st		fr5,56(r4),0,0
	//*/
	
	blr
	
	////////////////////////////////////
	
	.globl ps_guMtx44Concat
	//r3 = a, r4 = b, r5 = ab
ps_guMtx44Concat:
    psq_l       fr0, 0(r3), 0, 0;
    psq_l       fr2, 0(r4), 0, 0;
    ps_muls0    fr6, fr2, fr0;
    psq_l       fr3, 16(r4), 0, 0;
    psq_l       fr4, 32(r4), 0, 0;
    ps_madds1   fr6, fr3, fr0, fr6;
    psq_l       fr1, 8(r3), 0, 0;
    psq_l       fr5, 48(r4), 0, 0;
    ps_madds0   fr6, fr4, fr1, fr6;
    psq_l       fr0, 16(r3), 0, 0;
    ps_madds1   fr6, fr5, fr1, fr6;
    psq_l       fr1, 24(r3), 0, 0;
    ps_muls0    fr8, fr2, fr0;
    ps_madds1   fr8, fr3, fr0, fr8;
    psq_l       fr0, 32(r3), 0, 0;
    ps_madds0   fr8, fr4, fr1, fr8;
    ps_madds1   fr8, fr5, fr1, fr8;
    psq_l       fr1, 40(r3), 0, 0;
    ps_muls0    fr10, fr2, fr0;
    ps_madds1   fr10, fr3, fr0, fr10;
    psq_l       fr0, 48(r3), 0, 0;
    ps_madds0   fr10, fr4, fr1, fr10;
    ps_madds1   fr10, fr5, fr1, fr10;
    psq_l       fr1, 56(r3), 0, 0;
    ps_muls0    fr12, fr2, fr0;
    psq_l       fr2, 8(r4), 0, 0;
    ps_madds1   fr12, fr3, fr0, fr12;
    psq_l       fr0, 0(r3), 0, 0;
    ps_madds0   fr12, fr4, fr1, fr12;
    psq_l       fr3, 24(r4), 0, 0;
    ps_madds1   fr12, fr5, fr1, fr12;
    psq_l       fr1, 8(r3), 0, 0;
    ps_muls0    fr7, fr2, fr0;
    psq_l       fr4, 40(r4), 0, 0;
    ps_madds1   fr7, fr3, fr0, fr7;
    psq_l       fr5, 56(r4), 0, 0;
    ps_madds0   fr7, fr4, fr1, fr7;
    psq_l       fr0, 16(r3), 0, 0; 
    ps_madds1   fr7, fr5, fr1, fr7; 
    psq_l       fr1, 24(r3), 0, 0;
    ps_muls0    fr9, fr2, fr0;
    psq_st      fr6, 0(r5), 0, 0;
    ps_madds1   fr9, fr3, fr0, fr9;
    psq_l       fr0, 32(r3), 0, 0;
    ps_madds0   fr9, fr4, fr1, fr9;
    psq_st      fr8, 16(r5), 0, 0;
    ps_madds1   fr9, fr5, fr1, fr9;
    psq_l       fr1, 40(r3), 0, 0;
    ps_muls0    fr11, fr2, fr0;
    psq_st      fr10,32(r5), 0, 0;
    ps_madds1   fr11, fr3, fr0, fr11;
    psq_l       fr0, 48(r3), 0, 0;
    ps_madds0   fr11, fr4, fr1, fr11;
    psq_st      fr12, 48(r5), 0, 0;
    ps_madds1   fr11, fr5, fr1, fr11;
    psq_l       fr1, 56(r3), 0, 0;
    ps_muls0    fr13, fr2, fr0; 
    psq_st      fr7, 8(r5), 0, 0;
    ps_madds1   fr13, fr3, fr0, fr13;   
    psq_st      fr9, 24(r5), 0, 0;
    ps_madds0   fr13, fr4, fr1, fr13;
    psq_st      fr11, 40(r5), 0, 0;
    ps_madds1   fr13, fr5, fr1, fr13;
    psq_st      fr13, 56(r5), 0, 0;
    blr
	
	
	.globl ps_guMtx44ApplyTrans
	//r3 = src,r4 = dst,fr1 = xT,fr2 = yT,fr3 = zT
ps_guMtx44ApplyTrans:
	lis			r9,Unit01@ha
	addi		r9,r9,Unit01@l	
	lfs			fr6,4(r9)
	psq_l		fr4,0(r3),0,0
	frsp		fr1,fr1
	psq_l		fr5,8(r3),0,0
	frsp		fr2,fr2
	ps_merge00	fr10,fr1,fr2
	psq_l		fr7,24(r3),0,0
	frsp		fr3,fr3
	ps_mul		fr1,fr4,fr10
	ps_merge00	fr11,fr3,fr6
	psq_l		fr8,40(r3),0,0
	ps_madd		fr2,fr5,fr11,fr1
	psq_l		fr6,16(r3),0,0
	ps_sum0		fr3,fr2,fr3,fr2
	psq_l		fr9,32(r3),0,0
	ps_mul		fr12,fr6,fr10
	psq_st		fr4,0(r4),0,0
	ps_madd		fr4,fr7,fr11,fr12
	psq_st		fr5,8(r4),1,0
	ps_sum0		fr12,fr4,fr12,fr4
	psq_st		fr3,12(r4),1,0
	ps_mul		fr3,fr9,fr10
	psq_st		fr6,16(r4),0,0
	ps_madd		fr2,fr8,fr11,fr3
	psq_st		fr7,24(r4),1,0
	ps_sum0		fr3,fr2,fr3,fr2
	psq_st		fr12,28(r4),1,0
	psq_st		fr9,32(r4),0,0
	psq_st		fr8,40(r4),1,0
	psq_st		fr3,44(r4),1,0
	psq_l		fr6,48(r3),0,0
	psq_l		fr7,56(r3),0,0
	psq_st		fr6,48(r4),0,0
	ps_mul		fr4, fr6, fr10
	ps_madd		fr5, fr7, fr11, fr4
	ps_sum1		fr10, fr5, fr7, fr5
	psq_st		fr10,56(r4),0,0	
	
	blr
	
	.globl ps_guMtx44Scale
	//r3 = mtx44, fr1 = xS, fr2 = yS, fr3 = zS
ps_guMtx44Scale:
	lis			r9,Unit01@ha
	addi		r9,r9,Unit01@l
	// r9(0) = 0 , r9(1) = 1
	// fr0(0) = 0, fr0(0) = 0
	lfs			fr0,0(r9)
	stfs		fr1,0(r3)
	psq_st		fr0,4(r3),0,0
	psq_st		fr0,12(r3),0,0
	stfs		fr2,20(r3)
	psq_st		fr0,24(r3),0,0
	psq_st		fr0,32(r3),0,0
	stfs		fr3,40(r3)
	// fr4 = 1
	lfs			fr4, 4(r9)
    psq_st      fr0, 44(r3), 0, 0
    psq_st      fr0, 52(r3), 0, 0
    stfs        fr4, 60(r3)
	blr

	
	
	
	////////////////////////////////////
	
	//TODO: This is acting up
	
	.globl ps_guMtx44MultVec
	//r3 = Mtx44, r4 = vec, r5 = dstVec
ps_guMtx44MultVec:
	psq_l		fr0,0(r4),0,0
	psq_l		fr2,0(r3),0,0
	psq_l		fr1,8(r4),1,0	// THE ONLY CHANGE!
	ps_mul		fr4,fr2,fr0
	psq_l		fr3,8(r3),0,0
	ps_madd		fr5,fr3,fr1,fr4
	psq_l		fr8,16(r3),0,0
	ps_sum0		fr6,fr5,fr6,fr5
	psq_l		fr9,24(r3),0,0
	ps_mul		fr10,fr8,fr0
	psq_st		fr6,0(r5),1,0
	ps_madd		fr11,fr9,fr1,fr10
	psq_l		fr2,32(r3),0,0
	ps_sum0		fr12,fr11,fr12,fr11
	psq_l		fr3,40(r3),0,0
	ps_mul		fr4,fr2,fr0
	psq_st		fr12,4(r5),1,0
	ps_madd		fr5,fr3,fr1,fr4
	psq_l		fr8, 48(r3), 0, 0
	ps_sum0		fr6,fr5,fr6,fr5
	psq_l		fr9, 56(r3), 0, 0
	ps_mul		fr10, fr8, fr0
	psq_st		fr6,8(r5),1,0
	ps_madd		fr11, fr9, fr1, fr10
	ps_sum0		fr12, fr11, fr12, fr11
	psq_st		fr12, 12(r5), 1, 0
	blr
	
	
	
	// TAKE OUT!
	.globl ps_guMtxInvXpose_copyFromLibogc
	//r3 = src, r4 = invx
ps_guMtxInvXpose_copyFromLibogc:
	psq_l       fr0, 0(r3), 1, 0
	psq_l       fr1, 4(r3), 0, 0
	psq_l       fr2, 16(r3), 1, 0
	ps_merge10  fr6, fr1, fr0
	psq_l       fr3, 20(r3), 0, 0
	psq_l       fr4, 32(r3), 1, 0
	ps_merge10  fr7, fr3, fr2
	psq_l       fr5, 36(r3), 0, 0
	ps_mul      fr11, fr3, fr6
	ps_merge10  fr8, fr5, fr4
	ps_mul      fr13, fr5, fr7
	ps_msub     fr11, fr1, fr7, fr11
	ps_mul      fr12, fr1, fr8
	ps_msub     fr13, fr3, fr8, fr13
	ps_msub     fr12, fr5, fr6, fr12
	ps_mul      fr10, fr3, fr4
	ps_mul      fr9,  fr0, fr5
	ps_mul      fr8,  fr1, fr2
	ps_msub     fr10, fr2, fr5, fr10
	ps_msub     fr9,  fr1, fr4, fr9
	ps_msub     fr8,  fr0, fr3, fr8
	ps_mul      fr7, fr0, fr13
	ps_sub      fr1, fr1, fr1
	ps_madd     fr7, fr2, fr12, fr7
	ps_madd     fr7, fr4, fr11, fr7
	ps_cmpo0    cr0, fr7, fr1
	bne         0f
	addi        r3, 0, 0
	blr
0:	fres        fr0, fr7
	psq_st      fr1,  12(r4), 1, 0
	ps_add      fr6, fr0, fr0
	ps_mul      fr5, fr0, fr0
	psq_st      fr1,  28(r4), 1, 0
	ps_nmsub    fr0, fr7, fr5, fr6
	psq_st      fr1,  44(r4), 1, 0
	ps_muls0    fr13, fr13, fr0
	ps_muls0    fr12, fr12, fr0
	ps_muls0    fr11, fr11, fr0
	psq_st      fr13,  0(r4), 0, 0
	psq_st      fr12,  16(r4), 0, 0
	ps_muls0    fr10, fr10, fr0
	ps_muls0    fr9,  fr9,  fr0
	psq_st      fr11,  32(r4), 0, 0
	psq_st      fr10,  8(r4), 1, 0
	ps_muls0    fr8,  fr8,  fr0
	addi        r3, 0, 1
	psq_st      fr9,   24(r4), 1, 0
	psq_st      fr8,   40(r4), 1, 0
	blr
	
	
	.section .data
	.balign 4
Unit01:
	.float	0.0, 1.0
	