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
	Mtx44P pop();
	Mtx44P push(Mtx44 m);

	void clear();
	void setPosition(u32 pos);

	Mtx44P getStackPtr();
	Mtx44P getStackPtr(u32 index);

	u32 position();
	u32 size();
	
private:
	Mtx44* stackBase;
	const u8 maxMtx;	// The maximum number of matrices that we can use
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

MtxStack::MtxStack(u32 max):  
	// There is actually an extra, secret, unusable array.
	stackBase(new Mtx44[max + 1]),
	maxMtx(max + 1),
	curMtx(0){
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
    
	//if(stackBase)
		delete[] stackBase;
	
	stackBase = NULL;
}

//----------------------------------------
//
// Function: init
//
//    Initialize the matrix stack
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
    curMtx = 0;
}

//----------------------------------------
//
// Function: push
//
//    Copy a matrix to stack pointer.
//
// @pre     -
// @post    The matrix is pushed onto the stack
// @param   m: The matrix to copy into the current location 
// @return  Pointer to the top of the stack
//
//----------------------------------------

Mtx44P MtxStack::push(Mtx44 m){
    ++curMtx;
	// Check for stack overflow
    if(curMtx >= maxMtx){
        // We have TOO MANY matrices! 
        //return NULL; 
		curMtx = maxMtx - 1;
    }
    
    guMtx44Copy(m, stackBase[curMtx]);

    return (stackBase[curMtx]);
}

//----------------------------------------
//
// Function: pop
//
//    Decrement the stack and return the matrix on top
//
// @pre     -
// @post    The top of the stack has been decremented
// @param   -
// @return  Pointer to the top of the stack
//
//----------------------------------------

Mtx44P MtxStack::pop(){
	
    if(curMtx == 0){
        //return NULL;

        // Nothing to pop! Return something, maybe? 
        return (stackBase[0]);
    }
    --curMtx;
    return (stackBase[curMtx]);
}

//----------------------------------------
//
// Function: clear
//
//    Clears the stack
//
// @pre     -
// @post    All matrices are now unused.
// @param   -
// @return  -
//
//----------------------------------------

void MtxStack::clear(){

    if(curMtx != 0)
        MMU_new.gxstat.se = 1;
	// Should we initialize them all?
	init();

    //curMtx = 0;
	
}

//----------------------------------------
//
// Function: setPosition
//
//    Set the current position
//
// @pre     -
// @post    -
// @param   pos: The new position
// @return  -
//
//----------------------------------------

void MtxStack::setPosition(u32 pos){
	if(pos >= maxMtx){
		curMtx = maxMtx - 1;
	}else{
		curMtx = pos;
	}
}

//----------------------------------------
//
// Function: getStackPtr
//
//    Get the top of the stack (but don't pop)
//
// @pre     -
// @post    -
// @param   -
// @return  Pointer to the top of the stack
//
//----------------------------------------

Mtx44P MtxStack::getStackPtr(){
    return (stackBase[curMtx]);
}

//----------------------------------------
//
// Function: getStackPtr
//
//    Get the matrix at the specified point (but don't pop)
//
// @pre     -
// @post    -
// @param   index: Which matrix in the stack to return
// @return  Pointer to the specific matrix
//
//----------------------------------------

Mtx44P MtxStack::getStackPtr(u32 index){
	if(index > curMtx){
        // Oops, we're out of bounds
		return (stackBase[curMtx]);

        //return NULL;
    }
    return (stackBase[index]);
}

//----------------------------------------
//
// Function: position
//
//    Get the index of the current matrix
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
//    Get the total size of the stack
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

#endif
/*===============================================================*/
