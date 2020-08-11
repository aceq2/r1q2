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
#include "client.h"

/*

key up events are sent even if in console mode

*/

char	key_lines[32][MAXCMDLINE];
int		key_linepos;
int		shift_down=false;
int	anykeydown;

int		edit_line=0;
int		history_line=0;

//int		key_waiting;
char		*keybindings[256];
qboolean	consolekeys[256];	// if true, can't be rebound while in console
qboolean	menubound[256];	// if true, can't be rebound while in menu
qboolean	buttondown[256];
int			keyshift[256];		// key to map to if shift held down in console
int			key_repeats[256];	// if > 1, it is autorepeating
int			keydown[256];

unsigned int			key_lastrepeat[256];

int			key_repeatrate;
int			key_repeatdelay;

cvar_t	*cl_cmdcomplete;

typedef struct
{
	char	/*@null@*/ *name;
	int		keynum;
} keyname_t;

keyname_t keynames[] =
{
	{"TAB", K_TAB},
	{"ENTER", K_ENTER},
	{"ESCAPE", K_ESCAPE},
	{"SPACE", K_SPACE},
	{"BACKSPACE", K_BACKSPACE},
	{"UPARROW", K_UPARROW},
	{"DOWNARROW", K_DOWNARROW},
	{"LEFTARROW", K_LEFTARROW},
	{"RIGHTARROW", K_RIGHTARROW},

	{"ALT", K_ALT},
	{"CTRL", K_CTRL},
	{"SHIFT", K_SHIFT},
	
	{"F1", K_F1},
	{"F2", K_F2},
	{"F3", K_F3},
	{"F4", K_F4},
	{"F5", K_F5},
	{"F6", K_F6},
	{"F7", K_F7},
	{"F8", K_F8},
	{"F9", K_F9},
	{"F10", K_F10},
	{"F11", K_F11},
	{"F12", K_F12},

	{"INS", K_INS},
	{"DEL", K_DEL},
	{"PGDN", K_PGDN},
	{"PGUP", K_PGUP},
	{"HOME", K_HOME},
	{"END", K_END},

	{"MOUSE1", K_MOUSE1},
	{"MOUSE2", K_MOUSE2},
	{"MOUSE3", K_MOUSE3},
	{"MOUSE4", K_MOUSE4},
	{"MOUSE5", K_MOUSE5},
	{"MOUSE6", K_MOUSE6},
	{"MOUSE7", K_MOUSE7},
	{"MOUSE8", K_MOUSE8},
	{"MOUSE9", K_MOUSE9},

	{"JOY1", K_JOY1},
	{"JOY2", K_JOY2},
	{"JOY3", K_JOY3},
	{"JOY4", K_JOY4},

	{"AUX1", K_AUX1},
	{"AUX2", K_AUX2},
	{"AUX3", K_AUX3},
	{"AUX4", K_AUX4},
	{"AUX5", K_AUX5},
	{"AUX6", K_AUX6},
	{"AUX7", K_AUX7},
	{"AUX8", K_AUX8},
	{"AUX9", K_AUX9},
	{"AUX10", K_AUX10},
	{"AUX11", K_AUX11},
	{"AUX12", K_AUX12},
	{"AUX13", K_AUX13},
	{"AUX14", K_AUX14},
	{"AUX15", K_AUX15},
	{"AUX16", K_AUX16},
	{"AUX17", K_AUX17},
	{"AUX18", K_AUX18},
	{"AUX19", K_AUX19},
	{"AUX20", K_AUX20},
	{"AUX21", K_AUX21},
	{"AUX22", K_AUX22},
	{"AUX23", K_AUX23},
	{"AUX24", K_AUX24},
	{"AUX25", K_AUX25},
	{"AUX26", K_AUX26},
	{"AUX27", K_AUX27},
	{"AUX28", K_AUX28},
	{"AUX29", K_AUX29},
	{"AUX30", K_AUX30},
	{"AUX31", K_AUX31},
	{"AUX32", K_AUX32},

	{"KP_HOME",			K_KP_HOME },
	{"KP_UPARROW",		K_KP_UPARROW },
	{"KP_PGUP",			K_KP_PGUP },
	{"KP_LEFTARROW",	K_KP_LEFTARROW },
	{"KP_5",			K_KP_5 },
	{"KP_RIGHTARROW",	K_KP_RIGHTARROW },
	{"KP_END",			K_KP_END },
	{"KP_DOWNARROW",	K_KP_DOWNARROW },
	{"KP_PGDN",			K_KP_PGDN },
	{"KP_ENTER",		K_KP_ENTER },
	{"KP_INS",			K_KP_INS },
	{"KP_DEL",			K_KP_DEL },
	{"KP_SLASH",		K_KP_SLASH },
	{"KP_MINUS",		K_KP_MINUS },
	{"KP_PLUS",			K_KP_PLUS },

	{"NUMLOCK",			K_NUMLOCK },
	{"SCROLLLOCK",		K_SCROLLLOCK },
	{"CAPSLOCK",		K_CAPSLOCK },
	{"PRINTSCREEN",		K_PRTSCR },
	{"LWINKEY",			K_LWINKEY },
	{"RWINKEY",			K_RWINKEY },
	{"APP",				K_APP },

	{"MWHEELUP", K_MWHEELUP },
	{"MWHEELDOWN", K_MWHEELDOWN },

	{"PAUSE", K_PAUSE},

	{"SEMICOLON", ';'},	// because a raw semicolon seperates commands

	{NULL,0}
};

/*
==============================================================================

			LINE TYPING INTO THE CONSOLE

==============================================================================
*/

void Key_CompleteCommandOld (void)
{
	const char	*cmd;
	char		*s;

	s = key_lines[edit_line]+1;
	if (*s == '\\' || *s == '/')
		s++;

	cmd = Cmd_CompleteCommandOld (s);
	if (!cmd)
		cmd = Cvar_CompleteVariable (s);
	if (cmd)
	{
		key_lines[edit_line][1] = '/';
		strcpy (key_lines[edit_line]+2, cmd);
		key_linepos = (int)strlen(cmd)+2;
		key_lines[edit_line][key_linepos] = ' ';
		key_linepos++;
		key_lines[edit_line][key_linepos] = 0;
		return;
	}
}

/*
====================
Key_CompleteCommand
Stolen from Echon's EGL, echon.org but I'm sure he stole it from elsewhere :P
====================
*/
extern cmdalias_t	*cmd_alias;
extern cmd_function_t *cmd_functions;

static void Key_CompleteCommand (void) {
	cmd_function_t	*cmd, *cmdFound;
	cmdalias_t		*alias, *aliasFound;
	cvar_t			*cvar, *cvarFound;
	const char		*cmdName;
	char			*partial;
	int				aliasCnt, cmdCnt, cvarCnt, len, offset;
	qboolean		checkCmds = true;
	const char		*temp_cmd;
	const char		*temp_string;
	int				i;
	int				cmdLen;

	if (match_found) {
		if (match_found > 16)
			return;
		cmdName = NULL;
		temp_string = (char*)malloc(strlen(matchingcmds));
		strcpy(temp_string, matchingcmds);
		i = 0;
		temp_cmd = strtok(temp_string, "\n");
		if (cmdnr >= match_found)
			cmdnr = 0;
		while(i<cmdnr) {
			temp_cmd = strtok(NULL, "\n");
			i++;
		}
		free(temp_string);
		cmdnr++;
		cmdName = temp_cmd;
		goto cmd;
		/*//output command..
		cmdLen = (int)strlen(temp_cmd);
		if (cmdLen + 2 >= MAXCMDLINE)
		{
			Com_DPrintf("Key_CompleteCommand: expansion would overflow command buffer\n");
			return;
		}
		strcpy(key_lines[edit_line] + 1, temp_cmd);
		key_linepos = 1 + cmdLen;
		key_lines[edit_line][key_linepos] = ' ';
		key_linepos++;
		key_lines[edit_line][key_linepos] = 0;
		
		return;*/
	}
	
	partial = key_lines[edit_line]+1;
	// skip '/' and '\'
	if ((*partial == '\\') || (*partial == '/'))
	{
		offset = 1;
		partial++;
	}
	else
	{
		offset = 0;
	}

	//r1ch: hack set completion
	if (!Q_strncasecmp (partial, "set ", 4))
	{
		checkCmds = false;
		partial += 4;
		offset += 4;
	}
	else if (!Q_strncasecmp (partial, "cvarhelp ", 9))
	{
		checkCmds = false;
		partial += 9;
		offset += 9;
	}

	len = (int)strlen (partial);

	if (!len)
		return;

	cmdName = NULL;

	cmd = cmdFound = NULL;
	alias = aliasFound = NULL;
	cvar = cvarFound = NULL;
	aliasCnt = cmdCnt = cvarCnt = 0;

	//
	// check for exact match
	//
	if (checkCmds)
	{
		for (cmd=cmd_functions ; cmd ; cmd=cmd->next)
		{
			if (!Q_stricmp (partial, cmd->name)) {
				///if (cmdCnt == 0)
				//{
					cmdCnt++;
					cmdFound = cmd;
				//}
				/*if (cmd->compFrame != compFrame)
					cmdCnt++;
				cmd->compFrame = compFrame;*/
			}
		}

		for (alias=cmd_alias ; alias ; alias=alias->next)
		{
			if (!Q_stricmp (partial, alias->name)) {
				//if (aliasCnt == 0)
				//{
					aliasCnt++;
					aliasFound = alias;
				//}
				/*if (alias->compFrame != compFrame)
					aliasCnt++;
				alias->compFrame = compFrame;*/
			}
		}
	}

	for (cvar=cvar_vars ; cvar ; cvar=cvar->next)
	{
		if (!Q_stricmp (partial, cvar->name)) {
			//if (cvarCnt == 0)
			//{
				cvarFound = cvar;
				cvarCnt++;
			//}
			/*if (cvar->compFrame != compFrame)
				cvarCnt++;
			cvar->compFrame = compFrame;*/
		}
	}

	matchingcmds = (char *)malloc(16);
	sprintf(matchingcmds, "");
	//
	// check for partial match
	//
	if (checkCmds)
	{
		for (cmd=cmd_functions ; cmd ; cmd=cmd->next)
		{
			if (!Q_strncasecmp (partial, cmd->name, len)) {
				//if (cmdCnt == 0)
				//{
					matchingcmds = (char *)realloc(matchingcmds, strlen(matchingcmds) + cmd->name + 1);
					strcat(matchingcmds, cmd->name);
					strcat(matchingcmds, "\n");
					cmdFound = cmd;
					cmdCnt++;
					
				//}

				/*if (cmd->compFrame != compFrame)
					cmdCnt++;
				cmd->compFrame = compFrame;*/
			}
		}

		for (alias=cmd_alias ; alias ; alias=alias->next)
		{
			if (!Q_strncasecmp (partial, alias->name, len)) {
				//if (aliasCnt == 0)
				//{
					matchingcmds = (char *)realloc(matchingcmds, strlen(matchingcmds) + alias->name + 1);
					strcat(matchingcmds, alias->name);
					strcat(matchingcmds, "\n");
					aliasFound = alias;
					aliasCnt++;
				//}
				/*if (alias->compFrame != compFrame)
					aliasCnt++;
				alias->compFrame = compFrame;*/
			}
		}
	}

	for (cvar=cvar_vars ; cvar ; cvar=cvar->next)
	{
		if (!Q_strncasecmp (partial, cvar->name, len)) {
			//if (cvarCnt == 0)
			//{
				matchingcmds = (char *)realloc(matchingcmds, strlen(matchingcmds) + cvar->name + 1);
				strcat(matchingcmds, cvar->name);
				strcat(matchingcmds, "\n");
				cvarFound = cvar;
				cvarCnt++;
			//}
			/*if (cvar->compFrame != compFrame)
				cvarCnt++;
			cvar->compFrame = compFrame;*/
		}
	}

	match_found = aliasCnt + cmdCnt + cvarCnt;
	//
	// return a match if only one was found, otherwise list matches
	//
	if ((aliasCnt + cmdCnt + cvarCnt) == 1) {
		if (cmdFound)
			cmdName = cmdFound->name;
		else if (aliasFound)
			cmdName = aliasFound->name;
		else if (cvarFound)
			cmdName = cvarFound->name;
		else
			cmdName = NULL;
	} else {
		if (aliasCnt + cmdCnt + cvarCnt)
			Com_Printf ("\nFound %i possible matches:\n", LOG_CLIENT, match_found);
		/*
		if (aliasCnt > 0) {
			//Cbuf_AddText ("echo ----\n echo Matching aliases:\n echo ----\n");
			Cbuf_AddText (va ("aliaslist %s\n", partial));
		}

		if (cmdCnt > 0) {
			//Cbuf_AddText ("echo ----\n echo Matching commands:\n echo ----\n");
			Cbuf_AddText (va ("cmdlist %s\n", partial));
		}

		if (cvarCnt > 0) {
			//Cbuf_AddText ("echo ----\n echo Matching cvars:\n echo ----\n");
			Cbuf_AddText (va ("cvarlist %s\n", partial));
		}*/
		Com_Printf("%s", LOG_CLIENT, matchingcmds);
	}

cmd:
	if (cmdName)
	{
		cmdLen = (int)strlen(cmdName);
		if (cmdLen + offset + 2 >= MAXCMDLINE)
		{
			Com_DPrintf ("Key_CompleteCommand: expansion would overflow command buffer\n");
			return;
		}
		//key_lines[edit_line][1] = '/';
		//memmove (key_lines[edit_line]+offset+1, key_lines[edit_line]+offset+1+cmdLen, offset);
		strcpy (key_lines[edit_line]+offset+1, cmdName);
		key_linepos = 1 + offset + cmdLen;
		key_lines[edit_line][key_linepos] = ' ';
		key_linepos++;
		key_lines[edit_line][key_linepos] = 0;
	}
}

/*
====================
Key_Console

Interactive line editing and console scrollback
====================
*/
void Key_Console (int key)
{

	switch ( key )
	{
	case K_KP_SLASH:
		key = '/';
		break;
	case K_KP_MINUS:
		key = '-';
		break;
	case K_KP_PLUS:
		key = '+';
		break;
	case K_KP_HOME:
		key = '7';
		break;
	case K_KP_UPARROW:
		key = '8';
		break;
	case K_KP_PGUP:
		key = '9';
		break;
	case K_KP_LEFTARROW:
		key = '4';
		break;
	case K_KP_5:
		key = '5';
		break;
	case K_KP_RIGHTARROW:
		key = '6';
		break;
	case K_KP_END:
		key = '1';
		break;
	case K_KP_DOWNARROW:
		key = '2';
		break;
	case K_KP_PGDN:
		key = '3';
		break;
	case K_KP_INS:
		key = '0';
		break;
	case K_KP_DEL:
		key = '.';
		break;
	}

	if ( ( toupper( key ) == 'V' && keydown[K_CTRL] ) ||
		 ( ( ( key == K_INS ) || ( key == K_KP_INS ) ) && keydown[K_SHIFT] ) )
	{
		char *cbd;
		
		if ( ( cbd = Sys_GetClipboardData() ) != 0 )
		{
			int		i;
			char	*p, *s;

			p = cbd;
			while ((p = strchr (p, '\r')) != NULL)
				p[0] = ' ';

			//r1: multiline paste
			p = strrchr (cbd, '\n');
			if (p)
			{
				s = strrchr (p, '\n');
				if (s)
				{
					s[0] = 0;
					p++;
					if (cbd[0])
					{
						Cbuf_AddText (cbd);
						Cbuf_AddText ("\n");
						Com_Printf ("%s\n", LOG_GENERAL, cbd);
					}
				}
			}
			else
				p = cbd;
			
			i = (int)strlen( p );

			//r1: save byte for null terminator!!
			if ( i + key_linepos >= MAXCMDLINE - 1)
				i = MAXCMDLINE - key_linepos - 1;

			if ( i > 0 )
			{
				p[i]=0;
				strcat( key_lines[edit_line], p );
				key_linepos += i;
			}
			free( cbd );
		}

		return;
	}

	if ( key == 'l' ) 
	{
		if ( keydown[K_CTRL] )
		{
			Cbuf_AddText ("clear\n");
			return;
		}
	}

	if ( key == K_ENTER || key == K_KP_ENTER )
	{	// backslash text are commands, else chat
		if (key_lines[edit_line][1] == '\\' || key_lines[edit_line][1] == '/')
			Cbuf_AddText (key_lines[edit_line]+2);	// skip the >
		else
			Cbuf_AddText (key_lines[edit_line]+1);	// valid command

		Cbuf_AddText ("\n");
		Com_Printf ("%s\n", LOG_CLIENT,key_lines[edit_line]);
		edit_line = (edit_line + 1) & 31;
		history_line = edit_line;
		memset (key_lines[edit_line], 0, sizeof(key_lines[edit_line]));
		key_lines[edit_line][0] = ']';
		//key_lines[edit_line][1] = '\0';
		key_linepos = 1;
		if (cls.state == ca_disconnected)
			SCR_UpdateScreen ();	// force an update, because the command
									// may take some time
		return;
	}

	//r1: command completion stolen from proquake
	if (key == K_TAB)
	{	// command completion
		if (cl_cmdcomplete->intvalue == 1)
		{
			const char *cmd;
			int len, i;
			char *fragment;
			cvar_t *var;
			const char *best = "~";
			const char *least = "~";

			len = (int)strlen(key_lines[edit_line]);
			for (i = 0 ; i < len - 1 ; i++)
			{
				if (key_lines[edit_line][i] == ' ')
					return;
			}
			fragment = key_lines[edit_line] + 1;

			len--;
			for (var = cvar_vars->next ; var ; var = var->next)
			{
				if (strcmp(var->name, fragment) >= 0 && strcmp(best, var->name) > 0) {
					best = var->name;
				}
				if (strcmp(var->name, least) < 0)
					least = var->name;
			}
			cmd = Cmd_CompleteCommand(fragment);
			//if (strcmp(cmd, fragment) >= 0 && strcmp(best, cmd) > 0)
			if (cmd)
				best = cmd;
			if (best[0] == '~')
			{
				cmd = Cmd_CompleteCommand(" ");
				if (strcmp(cmd, least) < 0)
					best = cmd;
				else
					best = least;
			}

			//r1: maybe completing cvar/cmd from net?
			snprintf(key_lines[edit_line], sizeof(key_lines[edit_line])-1, "]%s ", best);
			key_lines[edit_line][sizeof(key_lines[edit_line])-1] = 0;

			key_linepos = (int)strlen(key_lines[edit_line]);
		}
		else if (cl_cmdcomplete->value == 2)
		{
			Key_CompleteCommand();
		}
		else
		{
			Key_CompleteCommandOld();
		}
		return;
	}
	
	if (match_found && key != K_TAB) // so it will not flood
	{
		free(matchingcmds);
		match_found = 0;
		cmdnr = 0;
	}

	if ( key == K_LEFTARROW )
	{
		if (key_linepos > 1)
			key_linepos--;
		return;
	}

	if (key == K_RIGHTARROW)
	{
		if (key_lines[edit_line][key_linepos])
			key_linepos++;
		return;
	}
	
	if ( ( key == K_BACKSPACE ) || ( ( key == 'h' ) && ( keydown[K_CTRL] ) ) )
	{
		if (key_linepos > 1)
		{
			memmove (key_lines[edit_line] + key_linepos-1, key_lines[edit_line] + key_linepos, sizeof(key_lines[edit_line])-key_linepos);
			key_linepos--;
		}
		return;
	}

	if ( key == K_DEL )
	{
		memmove (key_lines[edit_line] + key_linepos, key_lines[edit_line] + key_linepos + 1, sizeof(key_lines[edit_line])-key_linepos-1);
		return;
	}

	if ( ( key == K_UPARROW ) || ( key == K_KP_UPARROW ) ||
		 ( ( key == 'p' ) && keydown[K_CTRL] ) )
	{
		do
		{
			history_line = (history_line - 1) & 31;
		} while (history_line != edit_line
				&& !key_lines[history_line][1]);
		if (history_line == edit_line)
			history_line = (edit_line+1)&31;
		strcpy(key_lines[edit_line], key_lines[history_line]);
		key_linepos = (int)strlen(key_lines[edit_line]);
		return;
	}

	if ( ( key == K_DOWNARROW ) || ( key == K_KP_DOWNARROW ) ||
		 ( ( key == 'n' ) && keydown[K_CTRL] ) )
	{
		if (history_line == edit_line) return;
		do
		{
			history_line = (history_line + 1) & 31;
		}
		while (history_line != edit_line
			&& !key_lines[history_line][1]);
		if (history_line == edit_line)
		{
			key_lines[edit_line][0] = ']';
			key_lines[edit_line][1] = '\0';
			key_linepos = 1;
		}
		else
		{
			strcpy(key_lines[edit_line], key_lines[history_line]);
			key_linepos = (int)strlen(key_lines[edit_line]);
		}
		return;
	}

	if (key == K_PGUP || key == K_KP_PGUP )
	{
		con.display -= 2;
		return;
	}

	if (key == K_PGDN || key == K_KP_PGDN ) 
	{
		con.display += 2;
		if (con.display > con.current)
			con.display = con.current;
		return;
	}

	if (key == K_HOME || key == K_KP_HOME )
	{
		if (keydown[K_CTRL] || !key_lines[edit_line][1] || key_linepos == 1)
			con.display = con.current - con.totallines + 10;
		else
			key_linepos = 1;
		return;
	}

	if (key == K_END || key == K_KP_END )
	{
		int		len;

		len = (int)strlen(key_lines[edit_line]);

		if (keydown[K_CTRL] || !key_lines[edit_line][1] || key_linepos == len)
			con.display = con.current;
		else
			key_linepos = len;
		return;
	}
	
	if (key < 32 || key > 127)
		return;	// non printable
		
	if (key_linepos < MAXCMDLINE-1)
	{
		int		last;
		int		length;

		length = (int)strlen(key_lines[edit_line]);

		if (length >= MAXCMDLINE-1)
			return;

		last = key_lines[edit_line][length];

		memmove (key_lines[edit_line] + key_linepos+1, key_lines[edit_line] + key_linepos, length - key_linepos);

		key_lines[edit_line][key_linepos] = (char)key;
		key_linepos++;

		if (!last)
			key_lines[edit_line][length+1] = 0;
	}
}

//============================================================================

char		chat_custom_cmd[32];
char		chat_custom_prompt[32];
int			chat_mode;
char		chat_buffer[8][MAXCMDLINE];
int			chat_curbuffer = 0;
int			chat_bufferlen = 0;
int			chat_cursorpos = 0;
int			chat_editbuffer = 0;

void Key_Message (int key)
{
	char	last;

	if ( key == K_ENTER || key == K_KP_ENTER )
	{
		switch (chat_mode)
		{
		case CHAT_MODE_PUBLIC:
			Cbuf_AddText ("say \"");
			break;
		case CHAT_MODE_TEAM:
			Cbuf_AddText ("say_team \"");
			break;
		case CHAT_MODE_CUSTOM:
			Cbuf_AddText (chat_custom_cmd);
			Cbuf_AddText (" \"");
			break;
		default:
			Com_Error (ERR_DROP, "Bad chat_mode");
			break;
		}
		Cbuf_AddText(chat_buffer[chat_curbuffer]);
		Cbuf_AddText("\"\n");

		cls.key_dest = key_game;

		chat_curbuffer = (chat_curbuffer + 1) & 7;
		chat_editbuffer = chat_curbuffer;

		chat_bufferlen = 0;
		chat_buffer[chat_curbuffer][0] = 0;
		chat_cursorpos = 0;
		return;
	}

	if (key == K_UPARROW)
	{
		chat_curbuffer--;
		if (chat_curbuffer < 0)
			chat_curbuffer = 7;

		//ugly :E
		if (chat_curbuffer == chat_editbuffer)
		{
			chat_curbuffer++;
			if (chat_curbuffer > 7)
				chat_curbuffer = 0;
		}

		chat_bufferlen = chat_cursorpos = (int)strlen(chat_buffer[chat_curbuffer]);
		return;
	}

	if (key == K_DOWNARROW)
	{
		if (chat_curbuffer == chat_editbuffer)
			return;

		chat_curbuffer++;
		if (chat_curbuffer > 7)
			chat_curbuffer = 0;

		chat_bufferlen = chat_cursorpos = (int)strlen(chat_buffer[chat_curbuffer]);
		return;
	}

	if (key == K_ESCAPE)
	{
		cls.key_dest = key_game;
		chat_cursorpos = 0;
		chat_bufferlen = 0;
		chat_buffer[chat_curbuffer][0] = 0;
		return;
	}

	if (key == K_BACKSPACE)
	{
		if (chat_cursorpos)
		{
			//chat_bufferlen--;
			//chat_buffer[chat_bufferlen] = 0;
			memmove (chat_buffer[chat_curbuffer] + chat_cursorpos - 1, chat_buffer[chat_curbuffer] + chat_cursorpos, chat_bufferlen - chat_cursorpos + 1);
			chat_cursorpos--;
			chat_bufferlen--;
		}
		return;
	}

	if (key == K_DEL)
	{
		if (chat_bufferlen && chat_cursorpos != chat_bufferlen)
		{
			memmove (chat_buffer[chat_curbuffer] + chat_cursorpos, chat_buffer[chat_curbuffer] + chat_cursorpos + 1, chat_bufferlen - chat_cursorpos + 1);
			chat_bufferlen--;
		}
		return;
	}

	if (key == K_LEFTARROW)
	{
		if (chat_cursorpos > 0)
			chat_cursorpos--;
		return;
	}

	if (key == K_HOME)
	{
		chat_cursorpos = 0;
		return;
	}

	if (key == K_END)
	{
		chat_cursorpos = chat_bufferlen;
		return;
	}

	if (key == K_RIGHTARROW)
	{
		if (chat_buffer[chat_curbuffer][chat_cursorpos])
			chat_cursorpos++;
		return;
	}

	if (key < 32 || key > 127)
		return;	// non printable

	if (chat_bufferlen == sizeof(chat_buffer[chat_curbuffer])-1)
		return; // all full

	memmove (chat_buffer[chat_curbuffer] + chat_cursorpos + 1, chat_buffer[chat_curbuffer] + chat_cursorpos, chat_bufferlen - chat_cursorpos + 1);

	last = chat_buffer[chat_curbuffer][chat_cursorpos];

	chat_buffer[chat_curbuffer][chat_cursorpos] = (char)key;

	chat_bufferlen++;
	chat_cursorpos++;

	if (!last)
	{
		chat_buffer[chat_curbuffer][chat_cursorpos] = 0;
	}
}

//============================================================================


/*
===================
Key_StringToKeynum

Returns a key number to be used to index keybindings[] by looking at
the given string.  Single ascii characters return themselves, while
the K_* names are matched up.
===================
*/
int Key_StringToKeynum (char *str)
{
	keyname_t	*kn;
	
	if (!str || !str[0])
		return -1;
	if (!str[1])
		return str[0];

	for (kn=keynames ; kn->name ; kn++)
	{
		if (!Q_stricmp(str,kn->name))
			return kn->keynum;
	}
	return -1;
}

/*
===================
Key_KeynumToString

Returns a string (either a single ascii char, or a K_* name) for the
given keynum.
FIXME: handle quote special (general escape sequence?)
===================
*/
char *Key_KeynumToString (int keynum)
{
	keyname_t	*kn;	
	static	char	tinystr[2] = {0};
	
	if (keynum == -1)
		return "<KEY NOT FOUND>";

	if (keynum > 32 && keynum < 127)
	{	// printable ascii
		tinystr[0] = (char)keynum;
		//tinystr[1] = 0;
		return tinystr;
	}
	
	for (kn=keynames ; kn->name ; kn++)
		if (keynum == kn->keynum)
			return kn->name;

	return "<UNKNOWN KEYNUM>";
}


/*
===================
Key_SetBinding
===================
*/
void Key_SetBinding (int keynum, char *binding)
{
	char	*newKey;
			
	if (keynum == -1)
		return;

// free old bindings
	if (keybindings[keynum])
	{
		Z_Free (keybindings[keynum]);
		keybindings[keynum] = NULL;
	}
			
// allocate memory for new binding
	//l = strlen (binding);	
	//newKey = Z_TagMalloc (l+1, TAGMALLOC_CLIENT_KEYBIND);
	//strcpy (newKey, binding);
	//newKey[l] = 0;
	newKey = CopyString (binding, TAGMALLOC_CLIENT_KEYBIND);
	keybindings[keynum] = newKey;	
}

/*
===================
Key_Unbind_f
===================
*/
void Key_Unbind_f (void)
{
	int		b;

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("unbind <key> : remove commands from a key\n", LOG_CLIENT);
		return;
	}
	
	b = Key_StringToKeynum (Cmd_Argv(1));
	if (b==-1)
	{
		Com_Printf ("\"%s\" isn't a valid key\n", LOG_CLIENT, Cmd_Argv(1));
		return;
	}

	Key_SetBinding (b, "");
}

void Key_Unbindall_f (void)
{
	int		i;
	
	for (i=0 ; i<256 ; i++)
		if (keybindings[i])
			Key_SetBinding (i, "");
}


/*
===================
Key_Bind_f
===================
*/
void Key_Bind_f (void)
{
	int			i, c, b;
	char		cmd[1024];
	
	c = Cmd_Argc();

	if (c < 2)
	{
		Com_Printf ("bind <key> [command] : attach a command to a key\n", LOG_CLIENT);
		return;
	}
	b = Key_StringToKeynum (Cmd_Argv(1));
	if (b==-1)
	{
		Com_Printf ("\"%s\" isn't a valid key\n", LOG_CLIENT, Cmd_Argv(1));
		return;
	}

	if (c == 2)
	{
		if (keybindings[b])
			Com_Printf ("\"%s\" = \"%s\"\n", LOG_CLIENT, Cmd_Argv(1), keybindings[b] );
		else
			Com_Printf ("\"%s\" is not bound\n", LOG_CLIENT, Cmd_Argv(1) );
		return;
	}
	
// copy the rest of the command line
	cmd[0] = 0;		// start out with a null string
	for (i=2 ; i< c ; i++)
	{
		strcat (cmd, Cmd_Argv(i));
		if (i != (c-1))
			strcat (cmd, " ");
	}

	Key_SetBinding (b, cmd);
}

/*
============
Key_WriteBindings

Writes lines containing "bind key value"
============
*/
void Key_WriteBindings (FILE *f)
{
	int		i;

	for (i=0 ; i<256 ; i++)
		if (keybindings[i] && keybindings[i][0])
			fprintf (f, "bind %s \"%s\"\n", Key_KeynumToString(i), keybindings[i]);
}


/*
============
Key_Bindlist_f

============
*/
void Key_Bindlist_f (void)
{
	int		i;

	for (i=0 ; i<256 ; i++)
		if (keybindings[i] && keybindings[i][0])
			Com_Printf ("%s \"%s\"\n", LOG_CLIENT, Key_KeynumToString(i), keybindings[i]);
}


/*
===================
Key_Init
===================
*/
void Key_Init (void)
{
	int		i;

	for (i=0 ; i<32 ; i++)
	{
		key_lines[i][0] = ']';
		key_lines[i][1] = 0;
	}
	key_linepos = 1;
	
//
// init ascii characters in console mode
//
	for (i=32 ; i<128 ; i++)
		consolekeys[i] = true;
	consolekeys[K_ENTER] = true;
	consolekeys[K_KP_ENTER] = true;
	consolekeys[K_TAB] = true;
	consolekeys[K_LEFTARROW] = true;
	consolekeys[K_KP_LEFTARROW] = true;
	consolekeys[K_RIGHTARROW] = true;
	consolekeys[K_KP_RIGHTARROW] = true;
	consolekeys[K_UPARROW] = true;
	consolekeys[K_KP_UPARROW] = true;
	consolekeys[K_DOWNARROW] = true;
	consolekeys[K_KP_DOWNARROW] = true;
	consolekeys[K_BACKSPACE] = true;
	consolekeys[K_HOME] = true;
	consolekeys[K_KP_HOME] = true;
	consolekeys[K_END] = true;
	consolekeys[K_KP_END] = true;
	consolekeys[K_PGUP] = true;
	consolekeys[K_KP_PGUP] = true;
	consolekeys[K_PGDN] = true;
	consolekeys[K_KP_PGDN] = true;
	consolekeys[K_SHIFT] = true;
	consolekeys[K_INS] = true;
	consolekeys[K_KP_INS] = true;
	consolekeys[K_KP_DEL] = true;
	consolekeys[K_KP_SLASH] = true;
	consolekeys[K_KP_PLUS] = true;
	consolekeys[K_KP_MINUS] = true;
	consolekeys[K_KP_5] = true;

	consolekeys['`'] = true;
	consolekeys['~'] = true;
	consolekeys[K_DEL] = true;

	for (i=0 ; i<256 ; i++)
		keyshift[i] = i;
	for (i='a' ; i<='z' ; i++)
		keyshift[i] = i - 'a' + 'A';
	keyshift['1'] = '!';
	keyshift['2'] = '@';
	keyshift['3'] = '#';
	keyshift['4'] = '$';
	keyshift['5'] = '%';
	keyshift['6'] = '^';
	keyshift['7'] = '&';
	keyshift['8'] = '*';
	keyshift['9'] = '(';
	keyshift['0'] = ')';
	keyshift['-'] = '_';
	keyshift['='] = '+';
	keyshift[','] = '<';
	keyshift['.'] = '>';
	keyshift['/'] = '?';
	keyshift[';'] = ':';
	keyshift['\''] = '"';
	keyshift['['] = '{';
	keyshift[']'] = '}';
	keyshift['`'] = '~';
	keyshift['\\'] = '|';

	menubound[K_ESCAPE] = true;
	for (i=0 ; i<12 ; i++)
		menubound[K_F1+i] = true;

//
// register our functions
//
	Cmd_AddCommand ("bind",Key_Bind_f);
	Cmd_AddCommand ("unbind",Key_Unbind_f);
	Cmd_AddCommand ("unbindall",Key_Unbindall_f);
	Cmd_AddCommand ("bindlist",Key_Bindlist_f);

	cl_cmdcomplete = Cvar_Get ("cl_cmdcomplete", "2", 0); //changed to 2 as default..
}

void Key_GenerateRepeats (void)
{
	int		i;

	for (i = 0; i < 256; i++)
	{
		if (keydown[i])
		{
			if (curtime >= key_lastrepeat[i] + key_repeatrate)
			{
				Key_Event (i, true, curtime);
				key_lastrepeat[i] = curtime;
			}
		}
	}
}

/*
===================
Key_Event

Called by the system between frames for both key up and key down events
Should NOT be called during an interrupt!
===================
*/
void Key_Event (int key, qboolean down, uint32 time)
{
	char		*kb;
	char		cmd[1024];

	// hack for modal presses
	/*if (key_waiting == -1)
	{
		if (down)
			key_waiting = key;
		return;
		}*/

	//Com_Printf ("%d is %d for %u\n", LOG_GENERAL, key, down, time);

	// update auto-repeat status
	if (down)
	{
		key_repeats[key]++;
		if (cls.key_dest != key_console && key != K_BACKSPACE && key != K_DEL && key != K_LEFTARROW && key != K_RIGHTARROW
			&& key != K_PAUSE 
			&& key != K_PGUP 
			&& key != K_KP_PGUP 
			&& key != K_PGDN
			&& key != K_KP_PGDN
			&& key_repeats[key] > 1)
			return;	// ignore most autorepeats
			
		if (key >= 200 && !keybindings[key])
			Com_Printf ("%s is unbound, hit F4 to set.\n", LOG_CLIENT, Key_KeynumToString (key) );
	}
	else
	{
		key_repeats[key] = 0;
	}

	//for dinput
	if (down && keydown[K_ALT])
	{
		if (key == K_ENTER)
		{
			Com_Printf ("ALT+Enter, setting fullscreen %d.\n", LOG_CLIENT, !vid_fullscreen->intvalue);
			Cvar_SetValue( "vid_fullscreen", (float)!vid_fullscreen->intvalue );
			return;
		}
		else if (key == K_TAB)
		{
			//prevent executing action on alt+tab
			return;
		}
	}

	if (key == K_SHIFT)
		shift_down = down;

	// console key is hardcoded, so the user can never unbind it
	if ((key == '`' || key == '~') && !shift_down)
	{
		if (!down)
			return;
		Con_ToggleConsole_f ();
		return;
	}

	// any key during the attract mode will bring up the menu
	/*if (cl.attractloop && cls.key_dest != key_menu &&
		!(key >= K_F1 && key <= K_F12))
		key = K_ESCAPE;*/

	// menu key is hardcoded, so the user can never unbind it
	if (key == K_ESCAPE)
	{
		if (!down)
			return;

		if (cl.frame.playerstate.stats[STAT_LAYOUTS] && cls.key_dest == key_game)
		{	// put away help computer / inventory
			Cbuf_AddText ("cmd putaway\n");
			return;
		}
		switch (cls.key_dest)
		{
		case key_message:
			Key_Message (key);
			break;
		case key_menu:
			M_Keydown (key);
			break;
		case key_game:
		case key_console:
			M_Menu_Main_f ();
			break;
		default:
			Com_Error (ERR_FATAL, "Bad cls.key_dest");
		}
		return;
	}

	if (!keydown[key])
		key_lastrepeat[key] = curtime + key_repeatdelay;

	// track if any key is down for BUTTON_ANY
	keydown[key] = down;
	if (down)
	{
		if (key_repeats[key] == 1)
			anykeydown++;
	}
	else
	{
		key_lastrepeat[key] = 0;

		anykeydown--;
		if (anykeydown < 0)
			anykeydown = 0;
	}

//
// key up events only generate commands if the game key binding is
// a button command (leading + sign).  These will occur even in console mode,
// to keep the character from continuing an action started before a console
// switch.  Button commands include the kenum as a parameter, so multiple
// downs can be matched with ups
//
	if (!down)
	{
		//r1ch: only generate -events if key was down (prevents -binds in menu / messagemode)
		if (buttondown[key])
		{
			kb = keybindings[key];
			if (kb && kb[0] == '+')
			{
				Com_sprintf (cmd, sizeof(cmd), "-%s %i %u\n", kb+1, key, time);
				Cbuf_AddText (cmd);
			}
			if (keyshift[key] != key)
			{
				kb = keybindings[keyshift[key]];
				if (kb && kb[0] == '+')
				{
					Com_sprintf (cmd, sizeof(cmd), "-%s %i %u\n", kb+1, key, time);
					Cbuf_AddText (cmd);
				}
			}
			buttondown[key] = false;
		}
		return;
	}

//
// if not a consolekey, send to the interpreter no matter what mode is
//
	if ( (cls.key_dest == key_menu && menubound[key])
	|| (cls.key_dest == key_console && !consolekeys[key])
	|| (cls.key_dest == key_game && ( cls.state == ca_active || !consolekeys[key] ) ) )
	{
		kb = keybindings[key];
		if (kb && kb[0])
		{
			if (kb[0] == '+')
			{	
				// button commands add keynum and time as a parm
				if (cls.key_dest != key_game) //r1: don't run buttons in console
					return;
				Com_sprintf (cmd, sizeof(cmd), "%s %i %u\n", kb, key, time);
				Cbuf_AddText (cmd);
				buttondown[key] = true;
			}
			else
			{
				Cbuf_AddText (kb);
				Cbuf_AddText ("\n");
			}
		}
		return;
	}

	//if (!down)
	//	return;		// other systems only care about key down events

	if (shift_down)
		key = keyshift[key];

	switch (cls.key_dest)
	{
	case key_message:
		Key_Message (key);
		break;
	case key_menu:
		M_Keydown (key);
		break;

	case key_game:
	case key_console:
		Key_Console (key);
		break;
	default:
		Com_Error (ERR_FATAL, "Bad cls.key_dest");
	}
}

/*
===================
Key_ClearStates
===================
*/
void Key_ClearStates (void)
{
	int		i;

	anykeydown = false;

	for (i=0 ; i<256 ; i++)
	{
		if ( keydown[i] || key_repeats[i] )
			Key_Event( i, false, 0 );
		keydown[i] = 0;
		key_repeats[i] = 0;
		key_lastrepeat[i] = 0;
	}
}


/*
===================
Key_GetKey
===================
*/
/*int Key_GetKey (void)
{
	key_waiting = -1;

	while (key_waiting == -1)
		Sys_SendKeyEvents ();

	return key_waiting;
}*/

