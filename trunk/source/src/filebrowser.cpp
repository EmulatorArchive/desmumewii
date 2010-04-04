

#include "filebrowser.h"
#include <sys/dir.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <wiiuse/wpad.h>

#ifdef HW_RVL

#define GetInput(Wiimote, GC, Classic) \
		Input(WPAD_BUTTON_##Wiimote, PAD_BUTTON_##GC, WPAD_CLASSIC_BUTTON_##Classic)
		
static u32 Input(u32 Wiimote, u32 GC, u32 Classic)
{
	return ((WPAD_ButtonsDown(0) & Wiimote) || (PAD_ButtonsDown(0) & GC) || (WPAD_ButtonsDown(0) & Classic));
}

#define GetHeld(Wiimote, GC, Classic) \
		Held(WPAD_BUTTON_##Wiimote, PAD_BUTTON_##GC, WPAD_CLASSIC_BUTTON_##Classic)
		
static u32 Held(u32 Wiimote, u32 GC, u32 Classic)
{
	return ((WPAD_ButtonsHeld(0) & Wiimote) || (PAD_ButtonsHeld(0) & GC) || (WPAD_ButtonsHeld(0) & Classic));
}

#else	//!HW_RVL

#define GetInput(Wiimote, GC, Classic) \
		Input(PAD_BUTTON_##GC)

static u32 Input(u32 GC)
{
	return (PAD_ButtonsDown(0) & GC);
}

#define GetHeld(Wiimote, GC, Classic) \
		Held(PAD_BUTTON_##GC)

static u32 Held(u32 GC)
{
	return (PAD_ButtonsHeld(0) & GC);
}
#endif	//HW_RVL

typedef enum {
	BROWSER_FILE_NOT_FOUND	= -1,
	BROWSER_FILE_SELECTED	= 0,
	BROWSER_CANCELED	= 1,
	BROWSER_CHANGE_FOLDER	= 2
} ret_action;

typedef struct {
	char name[255];
	int  size;
	int  attr;
} dir_ent;

static int const per_page = 16;

typedef struct {
	char title[255];
	char path[MAXPATHLEN];
} file_browser_st;

static void clear_console()
{
	printf("\x1B[2J");
	VIDEO_WaitVSync();
	printf("\x1B[2;2H");
}

static void browse_back(char *str){
	int length = strlen(str);
	int idx;
	for( idx = length; idx > 0; idx-- ) {
		char ch = str[idx];
		str[idx] = '\0';
		if( ch == '/' ) {
			if( str[idx-1] == ':' )		// root folder.
				str[idx] = '/';		// Check is here, because it happens only once per function call.
			break;
		}
	}
}

static ret_action textFileBrowser(file_browser_st *file_struct, bool noslash){
	// Set everything up to read
	DIR_ITER* dp = diropen(file_struct->path);

	if(!dp)
		return BROWSER_FILE_NOT_FOUND;

	struct stat fstat;
	char filename[MAXPATHLEN];
	int num_entries = 1, i = 0;
	dir_ent* dir = (dir_ent*) malloc( num_entries * sizeof(dir_ent) );
	// Read each entry of the directory
	while( dirnext(dp, filename, &fstat) == 0 ){
		if( strcmp(filename, ".") != 0 )
		{
			// Make sure we have room for this one
			if(i == num_entries){
				++num_entries;
				dir = (dir_ent*) realloc( dir, num_entries * sizeof(dir_ent) ); 
			}
			strcpy(dir[i].name, filename);
			dir[i].size = fstat.st_size;
			dir[i].attr = fstat.st_mode;
			++i;
		}
	}

	dirclose(dp);

	int index	= 0;

	u8 page = 0, start, limit;
	u8 draw = 1;

	clear_console();
	while(1)
	{
		PAD_ScanPads();
#ifdef HW_RVL
		WPAD_ScanPads();
#endif
		if((WPAD_ButtonsHeld(0) & WPAD_BUTTON_HOME) || (PAD_ButtonsHeld(0) & PAD_TRIGGER_Z)) exit(0);

		if(GetHeld(UP, UP, UP))
		{
			if(index) index--;
			usleep(150000);
			draw = 1;
		}
		
		if(GetHeld(DOWN, DOWN, DOWN))
		{
			if(index < num_entries - 1) index++;
			usleep(150000);
			draw = 1;
		}

		if(GetInput(LEFT, LEFT, LEFT))
		{
			index = 0;
			draw = 1;
		}

		if(GetInput(RIGHT, RIGHT, RIGHT))
		{
			index = num_entries - 1;
			draw = 1;
		}

		if(GetInput(A, A, A))
		{
			if(index == 0 && strcmp(dir[index].name, "..") == 0) {
				browse_back(file_struct->path);
			}
			else {
                if(!noslash) 
				sprintf(file_struct->path, "%s/%s", file_struct->path, dir[index].name);
				if(noslash)
				sprintf(file_struct->path, "%s%s", file_struct->path, dir[index].name);
				}

			BOOL is_dir = (dir[index].attr & S_IFDIR);
			free(dir);
			if(is_dir) {
				return BROWSER_CHANGE_FOLDER;
			}
			else
				return BROWSER_FILE_SELECTED;
		}

		/*if(GetInput(B, B, B)) 
		{
			return BROWSER_CANCELED;
		}*/

		if(draw)
		{
			int temp = 0;
			u8 old_page = page;
			page = index / per_page;
			if( old_page != page ) {
				clear_console();
			}
			start = page * per_page;
			limit = ( num_entries > (start + per_page) ) ? ( start + per_page ) : num_entries;
			printf("\x1b[2J");
			printf("\x1B[2;2H");
			printf("\x1b[33m");
			printf("\t%s\n\n", file_struct->title);

			printf("\tbrowsing %s\n\n", file_struct->path);

			for(temp = start; temp < limit; temp++)
			{
				printf("\x1b[%um", (index == temp) ? 32 : 37);
				printf("\t%s\t%s\n", (dir[temp].attr & S_IFDIR) ? "DIR" : "   ", dir[temp].name);
			}

			printf("\x1b[37m");
			draw = 0;
		}
		VIDEO_WaitVSync();
	}
}

int FileBrowser( char *dir, bool device ) {
	int ret = 0;

	file_browser_st game_filename;
	strcpy(game_filename.title, "Welcome to DeSmuME Wii!\n\nWARNING! If you paid for this software, you have been scammed!");

	sprintf(game_filename.path, "%s", dir);

	ret = textFileBrowser(&game_filename, false);
	
	while(ret == BROWSER_CHANGE_FOLDER) {
		ret = textFileBrowser(&game_filename, false);
	}
	
	if (ret == BROWSER_FILE_SELECTED) {
		strcpy(dir, game_filename.path);
	}
	
	/*Jump to the root of the device to select a rom if root:/DSROM/ is not found*/
	if(ret != BROWSER_CHANGE_FOLDER && ret != BROWSER_FILE_SELECTED){
	file_browser_st game_filename2;
	strcpy(game_filename2.title, "Welcome to DeSmuME Wii!\n\nWARNING! If you paid for this software, you have been scammed!");
	if(!device)
	dir = "sd:/";
	if(device)
	dir = "usb:/";
	sprintf(game_filename2.path, "%s", dir);
	ret = textFileBrowser(&game_filename2, true);
    while(ret == BROWSER_CHANGE_FOLDER) {
		ret = textFileBrowser(&game_filename2, false);
	}
	
	if (ret == BROWSER_FILE_SELECTED) {
		strcpy(dir, game_filename2.path);
	}
	}

	clear_console();
	return ret;
}
