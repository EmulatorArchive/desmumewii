#include "types.h"
#include "emufile.h"

#include <vector>

bool EMUFILE::readAllBytes(std::vector<u8>* dstbuf, const std::string& fname)
{
	EMUFILE_FILE file(fname.c_str(),"rb");
	if(file.fail()) return false;
	int size = file.size();
	dstbuf->resize(size);
	file.fread(&dstbuf->at(0),size);
	return true;
}

size_t EMUFILE_MEMORY::_fread(const void *ptr, size_t bytes){
	if(len==0)
	{
		failbit = true;
		return 0;
	}
	u32 remain = len-pos;
	u32 todo = std::min<u32>(remain,(u32)bytes);
	
	if(todo<=4)
	{
		u8* src = buf()+pos;
		u8* dst = (u8*)ptr;
		for(int i=0;i<todo;i++)
			*dst++ = *src++;
	}
	else
	{
		memcpy((void*)ptr,buf()+pos,todo);
	}
	pos += todo;
	if(todo<bytes)
		failbit = true;
	return todo;
}