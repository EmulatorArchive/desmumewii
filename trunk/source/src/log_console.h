/*  Copyright (C) 2008 dhewg
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
 
#ifndef _LOG_CONSOLE_H_
#define _LOG_CONSOLE_H_
 
#include <gccore.h>
 
void log_console_init(GXRModeObj *vmode, u16 logsize, u16 x, u16 y, u16 w, u16 h);
void log_console_deinit(void);
void log_console_enable_log(bool enable);
void log_console_enable_video(bool enable);
 
#endif
 
