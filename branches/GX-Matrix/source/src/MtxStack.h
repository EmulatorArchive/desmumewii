
#ifndef _DesmumeWii_MtxStack_
#define _DesmumeWii_MtxStack_

#include <ogcsys.h>
#include <gccore.h>
#include "MtxMath.h"
#include "MMU.h"

class MtxStack{

public:
	MtxStack();
	MtxStack(u32 max);
	~MtxStack();

	void init();
	Mtx44* pop();
	Mtx44* pushInvXpose(Mtx44 m);
	Mtx44* pushInv(Mtx44 m);
	Mtx44* pushFwd(Mtx44 m);
	Mtx44* push(Mtx44 m);

	Mtx44* getStackPtr();
	Mtx44* getStackPtr(u32 index);

	u32 position();
	u32 size();

	// Copied:
	void adjustPosition(s32 pos);
	float GetMultipliedIndex (int index, float *matrix, float *rightMatrix);
	void setPosition(u32 pos);
	
private:
	Mtx44* stackBase;
	u8 maxMtx;			// The maximum number of matrices that we can use
	u8 curMtx;			// The number currently being used.
};



//----------------------------------------
//
// Default Constructor
//
// @pre     -
// @post    -
// @param   -
// @return  -
//
//----------------------------------------

MtxStack::MtxStack(): stackBase(NULL), maxMtx(0), curMtx(0){

	// This should NEVER be called!
	
}

//----------------------------------------
//
// Constructor
//
// @pre     -
// @post    A default stack with 31 matrices is created
// @param   max: The number of matrices to allocate for the stack
// @return  -
//
//----------------------------------------


MtxStack::MtxStack(u32 max){

	stackBase = new Mtx44[max]; 
	//stackBase = new Mtx[max-1];
    maxMtx   = max;
	curMtx   = 0;

}

//----------------------------------------
//
// Destructor
//
// @pre     -
// @post    The stack is gone!
// @param   -
// @return  -
//
//----------------------------------------


MtxStack::~MtxStack(){
    
	if(stackBase){
		delete[] stackBase;
	}
	stackBase = NULL;
}

//----------------------------------------
//
// Function: init
//
//	Initialize the matrix stack
//
// @pre     -
// @post    All the matrices are initialized with guMtx44Identity
// @param   -
// @return  -
//
//----------------------------------------


void MtxStack::init(){

	s32 i = maxMtx - 1;
	do{
		guMtx44Identity(stackBase[i]);
		--i;
	}while(i >= 0);

	// We have an "empty" matrix stack
	curMtx   = 0;

}


//----------------------------------------
//
// Function: push
//
//	Copy a matrix to stack pointer + 1.
//
// @pre     -
// @post    The matrix is pushed onto the stack
// @param   m: The matrix to copy into the current location 
// @return  Pointer to the top of the stack
//
//----------------------------------------

Mtx44* MtxStack::push(Mtx44 m){
	// Check for stack overflow
	if(curMtx >= maxMtx){
		// We have TOO MANY matrices! 
		// PANIC! PANIIIIIIIC!
		return NULL;	
	}
	guMtx44Copy(m, stackBase[curMtx]);
	curMtx++;

    return (&stackBase[curMtx-1]);
}

//----------------------------------------
//
// Function: pushFwd
//
//	Concatenate a matrix with the current top of the stack.
//	This is intended for use in building forward transformations.
//
// @pre     -
// @post    The matrix is pushed onto the stack
// @param   m: The matrix to copy into the current location 
// @return  Pointer to the top of the stack
//
//----------------------------------------

Mtx44* MtxStack::pushFwd(Mtx44 m){

	// Check for stack overflow
	if(curMtx >= maxMtx){
		// We have TOO MANY matrices! 
		// PANIC! PANIIIIIIIC!
		return NULL;	
	}
	guMtx44Concat(m, stackBase[curMtx], stackBase[curMtx]);
	curMtx++;

    return (&stackBase[curMtx-1]);
}

//----------------------------------------
//
// Function: pushInv
//
//	Concatenate an inverted matrix with the current top of the stack.
//	This is intended for use in building inverse transformations.
//
// @pre     -
// @post    The matrix is pushed onto the stack
// @param   m: The matrix to copy into the current location 
// @return  Pointer to the top of the stack
//
//----------------------------------------


Mtx44* MtxStack::pushInv(Mtx44 m){

    Mtx mInv;

    guMtxInverse(m, mInv);

	// Check for stack overflow
	if(curMtx >= maxMtx){
		// We have TOO MANY matrices! 
		// PANIC! PANIIIIIIIC!
		return NULL;	
	}
	guMtx44Concat(mInv, stackBase[curMtx], stackBase[curMtx]);
	curMtx++;

    return (&stackBase[curMtx-1]);
}

//----------------------------------------
//
// Function: pushInvXpose
//
//	Concatenate an inverse-transposed matrix with the current top of the stack.
//	This is intended for use in building inverse transformations.
//
// @pre     -
// @post    The matrix is pushed onto the stack
// @param   m: The matrix to copy into the current location 
// @return  Pointer to the top of the stack
//
//----------------------------------------


Mtx44* MtxStack::pushInvXpose(Mtx44 m){

    Mtx mIT;

	//guMtxInvXPose(m, mIT);
	//*
    guMtxInverse(     m, mIT );
    guMtxTranspose( mIT, mIT );
	//*/

	// Check for stack overflow
	if(curMtx >= maxMtx){
		// We have TOO MANY matrices! 
		// PANIC! PANIIIIIIIC!
		return NULL;
	}
	guMtx44Concat(mIT, stackBase[curMtx], stackBase[curMtx]);
	curMtx++;

	return (&stackBase[curMtx-1]);
	//--DCN: What about this?
	//return (&stackBase[curMtx++]);
}

//----------------------------------------
//
// Function: pop
//
//	Decrement the stack and return the matrix on top
//
// @pre     -
// @post    The top of the stack has been decremented
// @param   -
// @return  Pointer to the top of the stack
//
//----------------------------------------


Mtx44* MtxStack::pop(){

	if(curMtx == 0){
		return NULL;
	}else{
		--curMtx;
		return (&stackBase[curMtx]);
	}
}

//----------------------------------------
//
// Function: getStackPtr
//
//	Get the top of the stack (but don't pop)
//
// @pre     -
// @post    -
// @param   -
// @return  Pointer to the top of the stack
//
//----------------------------------------


Mtx44* MtxStack::getStackPtr(){
    return (&stackBase[curMtx]);
}

//----------------------------------------
//
// Function: getStackPtr
//
//	Get the matrix at the specified point (but don't pop)
//
// @pre     -
// @post    -
// @param   index: Which matrix in the stack to return
// @return  Pointer to the specific matrix
//
//----------------------------------------


Mtx44* MtxStack::getStackPtr(u32 index){
	if(index >= curMtx){
		// Oops, we're out of bounds
		return NULL;
	}
    return (&stackBase[index]);
}

//----------------------------------------
//
// Function: position
//
//	Get the index of the current matrix
//
// @pre     -
// @post    -
// @param   -
// @return  Index number of current matrix
//
//----------------------------------------


u32 MtxStack::position(){
	return curMtx;
}

//----------------------------------------
//
// Function: size
//
//	Get the total size of the stack
//
// @pre     -
// @post    -
// @param   -
// @return  Maximum number of matrices we can have in the stack
//
//----------------------------------------


u32 MtxStack::size(){
	return maxMtx;
}

//----------------------------------------
//
// Function: adjustPosition
//
//	Add or subtract to the current position
//
// @pre     -
// @post    -
// @param   pos: The number we add/sub to the current stack position
// @return  -
//
//----------------------------------------


void MtxStack::adjustPosition(s32 pos){

	s32 newPosition = curMtx + pos;

	//--DCN: Do tests, see if this will work on a u32:
	// (Or maybe just change it to a s32...)
	/*
	// Wraparound behavior
	s32 newPosition = curMtx;
	newPosition &= maxMtx;
	//*/

	//In the mean time, let's do it the old fashioned way:
	if(newPosition < 0){
		newPosition += maxMtx;
	}
	else if(u32(newPosition) >= maxMtx){
		newPosition -= maxMtx;
	}


	if(u32(newPosition) != curMtx)
		MMU_new.gxstat.se = 1;

	curMtx = u32(newPosition);
/*
	stack->position += pos;
	s32 newpos = stack->position;
	stack->position &= (stack->size);
//*/

}

//----------------------------------------
//
// Function: setPosition
//
//	Set the current position
//
// @pre     -
// @post    -
// @param   pos: The new position
// @return  -
//
//----------------------------------------


void MtxStack::setPosition(u32 pos){

	curMtx = pos;

}

//----------------------------------------
//
// Function: GetMultipliedIndex [NOT USED]
//
//	I don't know what this does.
//
// @pre     -
// @post    -
// @param   
// @return  -
//
//----------------------------------------
float MtxStack::GetMultipliedIndex (int index, float *matrix, float *rightMatrix){
	int iMod = index%4, iDiv = (index>>2)<<2;

	return	 (matrix[iMod   ]*rightMatrix[iDiv  ])
			+(matrix[iMod+ 4]*rightMatrix[iDiv+1])
			+(matrix[iMod+ 8]*rightMatrix[iDiv+2])
			+(matrix[iMod+12]*rightMatrix[iDiv+3]);
}

#endif
/*===============================================================*/
