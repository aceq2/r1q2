/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

//
// these are the key numbers that should be passed to Key_Event
//
#define	K_TAB			9
#define	K_ENTER			13
#define	K_ESCAPE		27
#define	K_SPACE			32

// normal keys should be passed as lowercased ascii

#define	K_BACKSPACE		127
#define	K_UPARROW		128
#define	K_DOWNARROW		129
#define	K_LEFTARROW		130
#define	K_RIGHTARROW	131

#define	K_ALT			132
#define	K_CTRL			133
#define	K_SHIFT			134
#define	K_F1			135
#define	K_F2			136
#define	K_F3			137
#define	K_F4			138
#define	K_F5			139
#define	K_F6			140
#define	K_F7			141
#define	K_F8			142
#define	K_F9			143
#define	K_F10			144
#define	K_F11			145
#define	K_F12			146
#define	K_INS			147
#define	K_DEL			148
#define	K_PGDN			149
#define	K_PGUP			150
#define	K_HOME			151
#define	K_END			152

#define K_KP_HOME		160
#define K_KP_UPARROW	161
#define K_KP_PGUP		162
#define	K_KP_LEFTARROW	163
#define K_KP_5			164
#define K_KP_RIGHTARROW	165
#define K_KP_END		166
#define K_KP_DOWNARROW	167
#define K_KP_PGDN		168
#define	K_KP_ENTER		169
#define K_KP_INS   		170
#define	K_KP_DEL		171
#define K_KP_SLASH		172
#define K_KP_MINUS		173
#define K_KP_PLUS		174

//DirectInput on Win32 provides these
#define	K_NUMLOCK		175
#define	K_SCROLLLOCK	176
#define	K_CAPSLOCK		177

#define	K_PRTSCR		178

#define	K_LWINKEY		179
#define	K_RWINKEY		180
#define K_APP			181

#define K_PAUSE			255

//
// mouse buttons generate virtual keys
//
#define	K_MOUSE1		200
#define	K_MOUSE2		201
#define	K_MOUSE3		202
#define	K_MOUSE4		203
#define	K_MOUSE5		204
#define	K_MOUSE6		205
#define	K_MOUSE7		206
#define	K_MOUSE8		207
#define	K_MOUSE9		208

//
// joystick buttons
//
#define	K_JOY1			209
#define	K_JOY2			210
#define	K_JOY3			211
#define	K_JOY4			212

//
// aux keys are for multi-buttoned joysticks to generate so they can use
// the normal binding process
//
#define	K_AUX1			213
#define	K_AUX2			214
#define	K_AUX3			215
#define	K_AUX4			216
#define	K_AUX5			217
#define	K_AUX6			218
#define	K_AUX7			219
#define	K_AUX8			220
#define	K_AUX9			221
#define	K_AUX10			222
#define	K_AUX11			223
#define	K_AUX12			224
#define	K_AUX13			225
#define	K_AUX14			226
#define	K_AUX15			227
#define	K_AUX16			228
#define	K_AUX17			229
#define	K_AUX18			230
#define	K_AUX19			231
#define	K_AUX20			232
#define	K_AUX21			233
#define	K_AUX22			234
#define	K_AUX23			235
#define	K_AUX24			236
#define	K_AUX25			237
#define	K_AUX26			238
#define	K_AUX27			239
#define	K_AUX28			240
#define	K_AUX29			241
#define	K_AUX30			242
#define	K_AUX31			243
#define	K_AUX32			244

#define K_MWHEELDOWN	245
#define K_MWHEELUP		246

extern char		*keybindings[256];
extern	int		key_repeats[256];

extern	int	anykeydown;
extern	char chat_buffer[8][MAXCMDLINE];
extern	int chat_bufferlen;
extern	int	chat_curbuffer;
extern	int	chat_cursorpos;

#define CHAT_MODE_PUBLIC	0
#define CHAT_MODE_TEAM		1
#define CHAT_MODE_CUSTOM	2

extern char		chat_custom_cmd[32];
extern char		chat_custom_prompt[32];
extern int			chat_mode;

void Key_GenerateRepeats (void);
void Key_Event (int key, qboolean down, uint32 time);
void Key_Init (void);
void Key_WriteBindings (FILE *f);
void Key_SetBinding (int keynum, char *binding);
void Key_ClearStates (void);
//int Key_GetKey (void);

