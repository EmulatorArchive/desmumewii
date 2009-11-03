#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <wiiuse/wpad.h>
//#include "FrontEnd.h"


typedef struct configparm{
	char name[32];
	long *var;
}configP;

configP configparms[30];
int totalconfig=0;


void InitConfigParms(){
	
}

int selposconfig=0;
void DisplayConfigParms(){
	int c;

	for (c=0;c<totalconfig;c++){
	 	
	    if(selposconfig == c)
		{
			
		}else {
			
		}
		
		
	}

}

int frameposconfig=0;
int langposconfig=0;

void DoConfig()
{

	

}

//5SM2SF


typedef struct fname{
	char name[256];
}f_name;

typedef struct flist{
	f_name fname[256];
	int cnt;
}f_list;

f_list filelist;

void ClearFileList(){
	filelist.cnt =0;
}


int HasExtension(char *filename){
	if(filename[strlen(filename)-4] == '.'){
		return 1;
	}
	return 0;
}


void GetExtension(const char *srcfile,char *outext){
	if(HasExtension((char *)srcfile)){
		strcpy(outext,srcfile + strlen(srcfile) - 3);
	}else{
		strcpy(outext,"");
	}
}

enum {
	EXT_NDS = 1,
	EXT_GZ = 2,
	EXT_ZIP = 4,
	EXT_UNKNOWN = 8,
};

const struct {
	char *szExt;
	int nExtId;
} stExtentions[] = {
	{"nds",EXT_NDS},
//	{"gz",EXT_GZ},
//	{"zip",EXT_ZIP},
	{NULL, EXT_UNKNOWN}
};

int getExtId(const char *szFilePath) {
	char *pszExt;

	if ((pszExt = strrchr(szFilePath, '.'))) {
		pszExt++;
		int i;
		for (i = 0; stExtentions[i].nExtId != EXT_UNKNOWN; i++) {
			if (!stricmp(stExtentions[i].szExt,pszExt)) {
				return stExtentions[i].nExtId;
			}
		}
	}

	return EXT_UNKNOWN;
}

void GetFileList(const char *root)
{
	
}


void DisplayFileList()
{
	
}

char app_path[128];
char romname[256];


void DSEmuGui(char *path,char *out)
{
	
}

