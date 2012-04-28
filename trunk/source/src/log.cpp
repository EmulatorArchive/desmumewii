/*
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

#ifndef GEKKO
# include <stdlib.h>
# include <string.h>
#endif
#include <stdio.h>
#include <stdarg.h>

#include "log.h"

// #define _FLOG

#ifdef _FLOG
static FILE *flog;
#endif

void Log_Init() {
#ifdef _FLOG
# ifdef GEKKO
	flog = fopen("sd:/dslog.log", "wb");
# else
	char *filename = getenv("HOME");
	strcat(filename, "/dslog.log");
	flog = fopen(filename, "wb");
# endif
#endif
}

void Log_DeInit() {
#ifdef _FLOG
	if(flog)
		fclose(flog);
#endif
}

void Log_fprintf(const char *msg, ...) {
#ifdef _FLOG
	va_list args;
	va_start(args, msg);
	vfprintf(flog, msg, args);
	va_end(args);
#endif
}
