/*
    Copyright (C) 2008 dhewg
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
 
#include <sys/iosupport.h>
#include <reent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#include "log_console.h"
 
static bool gecko = false;
static const devoptab_t *dot_video = NULL;
static VIRetraceCallback rcb = NULL;
static char **log = NULL;
static u16 log_size = 0;
static u16 log_next = 0;
static bool log_active = true;
static bool video_active = true;
 
static int __out_write(struct _reent *r, int fd, const char *ptr, size_t len) {
	 
	if (!ptr || len <= 0)
		return -1;
	 
	if (video_active) {
		dot_video->write_r(r, fd, ptr, len);
	} else {
		if (log_active) {
			u16 l = (log_next + 1) % log_size;
			if (log[l])
			free(log[l]);
			log[l] = strndup(ptr, len);
			 
			log_next = l;
		}
	}
 
	if (gecko)
		usb_sendbuffer(1, ptr, len);
	 
	return len;
}
 
const devoptab_t dot_out = {
"stdout", // device name
0, // size of file structure
NULL, // device open
NULL, // device close
__out_write,// device write
NULL, // device read
NULL, // device seek
NULL, // device fstat
NULL, // device stat
NULL, // device link
NULL, // device unlink
NULL, // device chdir
NULL, // device rename
NULL, // device mkdir
0, // dirStateSize
NULL, // device diropen_r
NULL, // device dirreset_r
NULL, // device dirnext_r
NULL, // device dirclose_r
NULL // device statvfs_r
};
 
void log_console_init(GXRModeObj *vmode, u16 logsize, u16 x, u16 y, u16 w, u16 h)
{
	 
	CON_InitEx(vmode, x, y, w, h);
	rcb = VIDEO_SetPostRetraceCallback(NULL);
	VIDEO_SetPostRetraceCallback(rcb);
	 
	gecko = usb_isgeckoalive(1);
	 
	if (log_size && log) {
		int i = log_size - 1;
		do{
			if (log[i]){
				free(log[i]);
			}
			--i;
		}while(i >= 0);
		 
		free(log);
	}
	 
	log_size = logsize;
	log_next = 0;
	 
	if (log_size) {
		log = (char **) malloc(log_size * sizeof(char *));
		
		int i = log_size - 1;
		do{
			log[i] = NULL;	
			--i;
		}while(i >= 0);
	}
	 
	log_active = log_size > 0;
	 
	dot_video = devoptab_list[STD_OUT];
	video_active = true;
	 
	devoptab_list[STD_OUT] = &dot_out;
	devoptab_list[STD_ERR] = &dot_out;
}
 
void log_console_deinit(void) {
	 
	if (log_size && log) {

		int i = log_size - 1;
		do{
			if (log[i]){
				free(log[i]);
			}
			--i;
		}while(i >= 0);
		 
		free(log);
		log = NULL;
	}
	 
	log_size = 0;
	log_next = 0;
	 
	devoptab_list[STD_OUT] = dot_video;
	devoptab_list[STD_ERR] = dot_video;
	 
	// VIDEO_SetPostRetraceCallback(rcb);
	 
	dot_video = NULL;
}
 
void log_console_enable_log(bool enable) {
	if (!log_size)
		return;
	 
	log_active = enable;
}
 
void log_console_enable_video(bool enable) {
	if (video_active == enable)
		return;

	video_active = enable;
	 
	if (enable)
		VIDEO_SetPostRetraceCallback(rcb);
	else
		VIDEO_SetPostRetraceCallback(NULL);
	 
	if (!enable || !log_size)
		return;

	struct _reent *r = _REENT;
	u16 l; 

	for (u16 i = 0; i < log_size; ++i) {
		l = (log_next + 1 + i) % log_size;
		if (log[l]) {
			dot_video->write_r(r, 0, log[l], strlen(log[l]));
			free(log[l]);
			log[l] = NULL;
		}
	}
	 
	fflush(stdout);
}
 
 
