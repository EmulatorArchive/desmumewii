/* joysdl.h - this file is part of DeSmuME
 *
 * Copyright (C) 2007 Pascal Giard
 *
 * Author: Pascal Giard <evilynux@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef CTRLSSDL_H
#define CTRLSSDL_H

#include <stdio.h>
#include <stdlib.h>
//#include <pthread.h>
#include <wiiuse/wpad.h>
#include "MMU.h"

#include "types.h"

#define ADD_KEY(keypad,key) ( (keypad) |= (key) )
#define RM_KEY(keypad,key) ( (keypad) &= ~(key) )
#define KEYMASK_(k)	(1 << (k))
#define JOY_AXIS_(k)    (((k)+1) << 8)

enum ds_keys {
	KEY_NONE = 0,
	KEY_A,
	KEY_B,
	KEY_SELECT,
	KEY_START,
	KEY_RIGHT,
	KEY_LEFT,
	KEY_UP,
	KEY_DOWN,
	KEY_R,
	KEY_L,
	KEY_X,
	KEY_Y,
	LAST_INPUT_BUTTON = KEY_Y,
	KEY_DEBUG,
	KEY_BOOST,
	NB_KEYS = KEY_BOOST
};

#ifdef HW_RVL

#define GetInput( Wiimote, GC, Classic ) \
	Input(WPAD_BUTTON_##Wiimote, PAD_BUTTON_##GC, WPAD_CLASSIC_BUTTON_##Classic)

#define Input( Wiimote, GC, Classic ) \
	((WPAD_ButtonsDown(0) & Wiimote) || (PAD_ButtonsDown(0) & GC) || (WPAD_ButtonsDown(0) & Classic))

#define GetHeld( Wiimote, GC, Classic ) \
	Held(WPAD_BUTTON_##Wiimote, PAD_BUTTON_##GC, WPAD_CLASSIC_BUTTON_##Classic)

#define Held( Wiimote, GC, Classic ) \
	((WPAD_ButtonsHeld(0) & Wiimote) || (PAD_ButtonsHeld(0) & GC) || (WPAD_ButtonsHeld(0) & Classic))

#else	//!HW_RVL

#define GetInput(Wiimote, GC, Classic) \
		Input(PAD_BUTTON_##GC)

#define Input(GC) \
	(PAD_ButtonsDown(0) & GC)

#define GetHeld(Wiimote, GC, Classic) \
		Held(PAD_BUTTON_##GC)

#define Held(GC) \
	(PAD_ButtonsHeld(0) & GC)

#endif	//HW_RVL

/* Keypad key names */
extern const char *key_names[NB_KEYS];
/* Current keyboard configuration */
extern u16 keyboard_cfg[NB_KEYS];
/* Current joypad configuration */
extern u16 joypad_cfg[NB_KEYS];
/* Number of detected joypads */
extern u16 nbr_joy;

#ifndef GTK_UI
struct mouse_status
{
  signed long x;
  signed long y;
  BOOL click;
  BOOL down;
};

extern mouse_status mouse;

void set_mouse_coord(signed long x,signed long y);
#endif // !GTK_UI

void load_default_config(const u16 kbCfg[]);
BOOL init_joy( void);
void uninit_joy( void);
void set_joy_keys(const u16 joyCfg[]);
void set_kb_keys(const u16 kbCfg[]);
u16 get_set_joy_key(int index);
void get_set_joy_axis(int index, int index_opp);
void update_keypad(u16 keys);
u16 get_keypad( void);
u16 lookup_key (u16 keyval);
u16 lookup_joy_key (u16 keyval);
void
process_ctrls_event( u16 *keypad,
                      float nds_screen_size_ratio);

void
process_joystick_events( u16 *keypad);

#endif /* CTRLSSDL_H */
