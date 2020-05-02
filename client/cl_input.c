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
// cl.input.c  -- builds an intended movement command to send to the server

#include "client.h"

cvar_t	*cl_nodelta;

extern	uint32	sys_frame_time;
uint32	frame_msec;
uint32	old_sys_frame_time;

/*
===============================================================================

KEY BUTTONS

Continuous button event tracking is complicated by the fact that two different
input sources (say, mouse button 1 and the control key) can both press the
same button, but the button should only be released when both of the
pressing key have been released.

When a key event issues a button command (+forward, +attack, etc), it appends
its key number as a parameter to the command so it can be matched up with
the release.

state bit 0 is the current state of the key
state bit 1 is edge triggered on the up to down transition
state bit 2 is edge triggered on the down to up transition


Key_Event (int key, qboolean down, uint32 time);

  +mlook src time

===============================================================================
*/


kbutton_t	in_klook;
kbutton_t	in_left, in_right, in_forward, in_back;
kbutton_t	in_lookup, in_lookdown, in_moveleft, in_moveright;
kbutton_t	in_strafe, in_speed, in_use, in_attack;
kbutton_t	in_up, in_down;

int			in_impulse;


void KeyDown (kbutton_t *b)
{
	int		k;
	char	*c;
	
	c = Cmd_Argv(1);
	if (c[0])
		k = atoi(c);
	else
		k = -1;		// typed manually at the console for continuous down

	if (k == b->down[0] || k == b->down[1])
		return;		// repeating key
	
	if (!b->down[0])
		b->down[0] = k;
	else if (!b->down[1])
		b->down[1] = k;
	else
	{
		Com_Printf ("Three keys down for a button!\n", LOG_CLIENT);
		return;
	}
	
	if (b->state & 1)
		return;		// still down

	// save timestamp
	c = Cmd_Argv(2);
	b->downtime = atoi(c);
	if (!b->downtime)
		b->downtime = sys_frame_time - 100;

	b->state |= 1 + 2;	// down + impulse down
}

void KeyUp (kbutton_t *b)
{
	int		k;
	char	*c;
	uint32	uptime;

	c = Cmd_Argv(1);
	if (c[0])
		k = atoi(c);
	else
	{ // typed manually at the console, assume for unsticking, so clear all
		b->down[0] = b->down[1] = 0;
		b->state = 4;	// impulse up
		return;
	}

	if (b->down[0] == k)
		b->down[0] = 0;
	else if (b->down[1] == k)
		b->down[1] = 0;
	else
		return;		// key up without coresponding down (menu pass through)
	if (b->down[0] || b->down[1])
		return;		// some other key is still holding it down

	if (!(b->state & 1))
		return;		// still up (this should not happen)

	// save timestamp
	c = Cmd_Argv(2);
	uptime = atoi(c);
	if (uptime)
		b->msec += uptime - b->downtime;
	else
		b->msec += 10;

	b->state &= ~1;		// now up
	b->state |= 4; 		// impulse up
}

void IN_KLookDown (void) {KeyDown(&in_klook);}
void IN_KLookUp (void) {KeyUp(&in_klook);}
void IN_UpDown(void) {KeyDown(&in_up);}
void IN_UpUp(void) {KeyUp(&in_up);}
void IN_DownDown(void) {KeyDown(&in_down);}
void IN_DownUp(void) {KeyUp(&in_down);}
void IN_LeftDown(void) {KeyDown(&in_left);}
void IN_LeftUp(void) {KeyUp(&in_left);}
void IN_RightDown(void) {KeyDown(&in_right);}
void IN_RightUp(void) {KeyUp(&in_right);}
void IN_ForwardDown(void) {KeyDown(&in_forward);}
void IN_ForwardUp(void) {KeyUp(&in_forward);}
void IN_BackDown(void) {KeyDown(&in_back);}
void IN_BackUp(void) {KeyUp(&in_back);}
void IN_LookupDown(void) {KeyDown(&in_lookup);}
void IN_LookupUp(void) {KeyUp(&in_lookup);}
void IN_LookdownDown(void) {KeyDown(&in_lookdown);}
void IN_LookdownUp(void) {KeyUp(&in_lookdown);}
void IN_MoveleftDown(void) {KeyDown(&in_moveleft);}
void IN_MoveleftUp(void) {KeyUp(&in_moveleft);}
void IN_MoverightDown(void) {KeyDown(&in_moveright);}
void IN_MoverightUp(void) {KeyUp(&in_moveright);}

void IN_SpeedDown(void) {KeyDown(&in_speed);}
void IN_SpeedUp(void) {KeyUp(&in_speed);}
void IN_StrafeDown(void) {KeyDown(&in_strafe);}
void IN_StrafeUp(void) {KeyUp(&in_strafe);}

void IN_AttackDown(void) {KeyDown(&in_attack);}
void IN_AttackUp(void) {KeyUp(&in_attack);}

void IN_UseDown (void) {KeyDown(&in_use);}
void IN_UseUp (void) {KeyUp(&in_use);}

void IN_Impulse (void) {in_impulse=atoi(Cmd_Argv(1));}

/*
===============
CL_KeyState

Returns the fraction of the frame that the key was down
===============
*/
float CL_KeyState (kbutton_t *key)
{
	float		val;
	int			msec;

	key->state &= 1;		// clear impulses

	msec = key->msec;
	key->msec = 0;

	if (key->state)
	{	// still down
		msec += sys_frame_time - key->downtime;
		key->downtime = sys_frame_time;
	}

	val = (float)msec / frame_msec;
	
	/*if (msec)
	{
		Com_Printf ("%i/%u -> %f\n", LOG_CLIENT, msec, frame_msec, val);
	}*/

	if (FLOAT_LT_ZERO(val))
		val = 0;

	if (val > 1)
		val = 1;

	return val;
}




//==========================================================================

cvar_t	*cl_upspeed;
cvar_t	*cl_forwardspeed;
cvar_t	*cl_sidespeed;

cvar_t	*cl_yawspeed;
cvar_t	*cl_pitchspeed;

cvar_t	*cl_run;

cvar_t	*cl_anglespeedkey;


/*
================
CL_AdjustAngles

Moves the local angle positions
================
*/
	//jec - merged this into CL_BaseMove

/*
================
CL_BaseMove

Applies keyboard input to usercmd
================
*/
//jec - rearranged this function to work with altered usercmd code

void CL_BaseMove (usercmd_t *cmd)
{	
	int i;
	float	tspeed;
	float	mspeed;
	
	//adjust for turning speed
	if (in_speed.state & 1)
		tspeed = (frame_msec / 1000.0f) * cl_anglespeedkey->value;
	else
		tspeed = frame_msec / 1000.0f;
	
	//adjust for running speed
	if ( (in_speed.state & 1) ^ cl_run->intvalue)
		mspeed = 2;
	else
		mspeed = 1;
	
	//handle left/right on keyboard
	i = cls.netchan.outgoing_sequence & (CMD_BACKUP-1);
	cmd = &cl.cmds[i];
	cl.cmd_time[i] = cls.realtime;	// for netgraph ping calculation
	
	if (in_strafe.state & 1)
	{	// keyboard strafe
		cmd->sidemove += (int16)(mspeed * cl_sidespeed->value * CL_KeyState (&in_right));
		cmd->sidemove -= (int16)(mspeed * cl_sidespeed->value * CL_KeyState (&in_left));
	}
	else
	{	// keyboard turn
		cl.viewangles[YAW] -= tspeed*cl_yawspeed->value*CL_KeyState (&in_right);
		cl.viewangles[YAW] += tspeed*cl_yawspeed->value*CL_KeyState (&in_left);
	}	

	//handle foward/back on keyboard
	if (in_klook.state & 1)
	{	// keyboard look
		cl.viewangles[PITCH] -= tspeed*cl_pitchspeed->value * CL_KeyState (&in_forward);
		cl.viewangles[PITCH] += tspeed*cl_pitchspeed->value * CL_KeyState (&in_back);
	}	
	else
	{	// keyboard move front/back
		cmd->forwardmove += (int16)(mspeed * cl_forwardspeed->value * CL_KeyState (&in_forward));
		cmd->forwardmove -= (int16)(mspeed * cl_forwardspeed->value * CL_KeyState (&in_back));
	}
	
	// keyboard look up/down
	cl.viewangles[PITCH] -= tspeed*cl_pitchspeed->value * CL_KeyState(&in_lookup);
	cl.viewangles[PITCH] += tspeed*cl_pitchspeed->value * CL_KeyState(&in_lookdown);
	
	// keyboard strafe left/right
	cmd->sidemove += (int16)(mspeed * cl_sidespeed->value * CL_KeyState (&in_moveright));
	cmd->sidemove -= (int16)(mspeed * cl_sidespeed->value * CL_KeyState (&in_moveleft));

	// keyboard jump/crouch
	cmd->upmove += (int16)(mspeed * cl_upspeed->value * CL_KeyState (&in_up));
	cmd->upmove -= (int16)(mspeed * cl_upspeed->value * CL_KeyState (&in_down));

	//r1ch: cap to max allowed ranges
	if (cmd->forwardmove > cl_forwardspeed->value * mspeed)
		cmd->forwardmove = (int16)(cl_forwardspeed->value * mspeed);
	else if (cmd->forwardmove < -cl_forwardspeed->value * mspeed)
		cmd->forwardmove = -(int16)(cl_forwardspeed->value * mspeed);

	if (cmd->sidemove > cl_sidespeed->value * mspeed)
		cmd->sidemove = (int16)(cl_sidespeed->value * mspeed);
	else if (cmd->sidemove < -cl_sidespeed->value * mspeed)
		cmd->sidemove = -(int16)(cl_sidespeed->value * mspeed);

	if (cmd->upmove > cl_upspeed->value * mspeed)
		cmd->upmove = (int16)(cl_upspeed->value * mspeed);
	else if (cmd->upmove < -cl_upspeed->value * mspeed)
		cmd->upmove = -(int16)(cl_upspeed->value * mspeed);
}

void CL_ClampPitch (void)
{
	float	pitch;

	pitch = SHORT2ANGLE(cl.frame.playerstate.pmove.delta_angles[PITCH]);
	if (pitch > 180)
		pitch -= 360;

	if (cl.viewangles[PITCH] + pitch < -360)
		cl.viewangles[PITCH] += 360; // wrapped
	if (cl.viewangles[PITCH] + pitch > 360)
		cl.viewangles[PITCH] -= 360; // wrapped

	if (cl.viewangles[PITCH] + pitch > 89)
		cl.viewangles[PITCH] = 89 - pitch;
	if (cl.viewangles[PITCH] + pitch < -89)
		cl.viewangles[PITCH] = -89 - pitch;
}

/*
==============
CL_FinishMove
==============
*/
	//jec - merged this into CL_RefreshCmd and CL_FinalizeCmd

/*
=================
CL_CreateCmd
=================
*/
//jec - this function has been split into three:

// CL_InitCmd
//jec - initialize the pending usercmd structure for use
__inline void CL_InitCmd (void)
{
	usercmd_t *cmd = &cl.cmds[ cls.netchan.outgoing_sequence & (CMD_BACKUP-1) ];

	// init the current cmd buffer
	memset(cmd, 0, sizeof(*cmd));
}

// CL_RefreshCmd
//jec - adds any new input changes to usercmd
//	that occurred since last Init or RefreshCmd
trace_t		EXPORT CL_PMTrace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end);
void CL_RefreshCmd (void)
{	
	int ms;
	usercmd_t *cmd = &cl.cmds[ cls.netchan.outgoing_sequence & (CMD_BACKUP-1) ];

	//get delta for this sample.
	frame_msec = sys_frame_time - old_sys_frame_time;	

	// bounds checking
	if (frame_msec < 1)
		return;

	if (frame_msec > 200)
		frame_msec = 200;

	//cmd->forwardmove = cmd->sidemove = cmd->upmove = 0;

	// get basic movement from keyboard
	CL_BaseMove (cmd);

	// allow mice or other external controllers to add to the move
	IN_Move (cmd);

	// update cmd viewangles for CL_PredictMove
	CL_ClampPitch ();

	cmd->angles[0] = ANGLE2SHORT(cl.viewangles[0]);
	cmd->angles[1] = ANGLE2SHORT(cl.viewangles[1]);
	cmd->angles[2] = ANGLE2SHORT(cl.viewangles[2]);

	// update cmd->msec for CL_PredictMove
	ms = (int)(cls.frametime * 1000);
	if (ms > 250)
		ms = 100;

	cmd->msec = (byte)ms;

	//update counter
	old_sys_frame_time = sys_frame_time;


	//7 = starting attack 1  2  4
	//5 = during attack   1     4 
	//4 = idle                  4

	//r1: send packet immediately on important events
	if (((in_attack.state & 2) || (in_use.state & 2)))
		send_packet_now = true;
}

// CL_FinalizeCmd
// jec - prepares usercmd for transmission,
//	adds all changes that occurred since last Init
void CL_FinalizeCmd (void)
{
	usercmd_t *cmd = &cl.cmds[ cls.netchan.outgoing_sequence & (CMD_BACKUP-1) ];

	//set any button hits that occured since last frame
	if ( in_attack.state & 3 )
		cmd->buttons |= BUTTON_ATTACK;

	in_attack.state &= ~2;

	if (in_use.state & 3)
		cmd->buttons |= BUTTON_USE;

	in_use.state &= ~2;

	if (anykeydown && cls.key_dest == key_game)
		cmd->buttons |= BUTTON_ANY;

	//...
	cmd->impulse = (byte)in_impulse;
	in_impulse = 0;

	//Com_Printf ("up:%d, side:%d f:%d\n", LOG_CLIENT, cmd->upmove, cmd->sidemove, cmd->forwardmove);

	//r1ch: cap forwardmove/etc to reasonable levels, sure it may be a short
	//but pmove on client/server caps at 300 total velocity so there is little
	//value in having these higher, all it does it make for less efficient
	//deltas.

	//update (7588+), we use 400 since water move physics is "buggy" in that it uses uncapped usercmd
	//values, but it's been gameplay since q2 began, so keep with it to be compatible.
	if (cmd->forwardmove > 400)
		cmd->forwardmove = 400;
	else if (cmd->forwardmove < -400)
		cmd->forwardmove = -400;

	if (cmd->sidemove > 400)
		cmd->sidemove = 400;
	else if (cmd->sidemove < -400)
		cmd->sidemove = -400;

	if (cmd->upmove > 400)
		cmd->upmove = 400;
	else if (cmd->upmove < -400)
		cmd->upmove = -400;

	// set the ambient light level at the player's current position
	cmd->lightlevel = (byte)cl_lightlevel->value;
}


void IN_CenterView (void)
{
	cl.viewangles[PITCH] = -SHORT2ANGLE(cl.frame.playerstate.pmove.delta_angles[PITCH]);
}

/*
============
CL_InitInput
============
*/
void CL_InitInput (void)
{
	Cmd_AddCommand ("centerview",IN_CenterView);

	Cmd_AddCommand ("+moveup",IN_UpDown);
	Cmd_AddCommand ("-moveup",IN_UpUp);
	Cmd_AddCommand ("+movedown",IN_DownDown);
	Cmd_AddCommand ("-movedown",IN_DownUp);
	Cmd_AddCommand ("+left",IN_LeftDown);
	Cmd_AddCommand ("-left",IN_LeftUp);
	Cmd_AddCommand ("+right",IN_RightDown);
	Cmd_AddCommand ("-right",IN_RightUp);
	Cmd_AddCommand ("+forward",IN_ForwardDown);
	Cmd_AddCommand ("-forward",IN_ForwardUp);
	Cmd_AddCommand ("+back",IN_BackDown);
	Cmd_AddCommand ("-back",IN_BackUp);
	Cmd_AddCommand ("+lookup", IN_LookupDown);
	Cmd_AddCommand ("-lookup", IN_LookupUp);
	Cmd_AddCommand ("+lookdown", IN_LookdownDown);
	Cmd_AddCommand ("-lookdown", IN_LookdownUp);
	Cmd_AddCommand ("+strafe", IN_StrafeDown);
	Cmd_AddCommand ("-strafe", IN_StrafeUp);
	Cmd_AddCommand ("+moveleft", IN_MoveleftDown);
	Cmd_AddCommand ("-moveleft", IN_MoveleftUp);
	Cmd_AddCommand ("+moveright", IN_MoverightDown);
	Cmd_AddCommand ("-moveright", IN_MoverightUp);
	Cmd_AddCommand ("+speed", IN_SpeedDown);
	Cmd_AddCommand ("-speed", IN_SpeedUp);
	Cmd_AddCommand ("+attack", IN_AttackDown);
	Cmd_AddCommand ("-attack", IN_AttackUp);
	Cmd_AddCommand ("+use", IN_UseDown);
	Cmd_AddCommand ("-use", IN_UseUp);
	Cmd_AddCommand ("impulse", IN_Impulse);
	Cmd_AddCommand ("+klook", IN_KLookDown);
	Cmd_AddCommand ("-klook", IN_KLookUp);

	cl_nodelta = Cvar_Get ("cl_nodelta", "0", 0);
}

/*
================
CL_AdjustAngles

Moves the local angle positions
================
*/
void CL_AdjustAngles (void)
{
	float	speed;
	float	up, down;
	
	if (in_speed.state & 1)
		speed = cls.frametime * cl_anglespeedkey->value;
	else
		speed = cls.frametime;

	if (!(in_strafe.state & 1))
	{
		cl.viewangles[YAW] -= speed*cl_yawspeed->value*CL_KeyState (&in_right);
		cl.viewangles[YAW] += speed*cl_yawspeed->value*CL_KeyState (&in_left);
	}
	if (in_klook.state & 1)
	{
		cl.viewangles[PITCH] -= speed*cl_pitchspeed->value * CL_KeyState (&in_forward);
		cl.viewangles[PITCH] += speed*cl_pitchspeed->value * CL_KeyState (&in_back);
	}
	
	up = CL_KeyState (&in_lookup);
	down = CL_KeyState(&in_lookdown);
	
	cl.viewangles[PITCH] -= speed*cl_pitchspeed->value * up;
	cl.viewangles[PITCH] += speed*cl_pitchspeed->value * down;
}

void CL_BaseMove_Synchronous (usercmd_t *cmd)
{	
	CL_AdjustAngles ();
	
	memset (cmd, 0, sizeof(*cmd));
	
	cmd->angles[0] = (int16)cl.viewangles[0];
	cmd->angles[1] = (int16)cl.viewangles[1];
	cmd->angles[2] = (int16)cl.viewangles[2];

	if (in_strafe.state & 1)
	{
		cmd->sidemove += (int16)(cl_sidespeed->value * CL_KeyState (&in_right));
		cmd->sidemove -= (int16)(cl_sidespeed->value * CL_KeyState (&in_left));
	}

	cmd->sidemove += (int16)(cl_sidespeed->value * CL_KeyState (&in_moveright));
	cmd->sidemove -= (int16)(cl_sidespeed->value * CL_KeyState (&in_moveleft));

	cmd->upmove += (int16)(cl_upspeed->value * CL_KeyState (&in_up));
	cmd->upmove -= (int16)(cl_upspeed->value * CL_KeyState (&in_down));

	if (! (in_klook.state & 1) )
	{	
		cmd->forwardmove += (int16)(cl_forwardspeed->value * CL_KeyState (&in_forward));
		cmd->forwardmove -= (int16)(cl_forwardspeed->value * CL_KeyState (&in_back));
	}	

//
// adjust for speed key / running
//
	if ( (in_speed.state & 1) ^ (int)(cl_run->value) )
	{
		cmd->forwardmove *= 2;
		cmd->sidemove *= 2;
		cmd->upmove *= 2;
	}	
}

void CL_FinishMove (usercmd_t *cmd)
{
	int		ms;
	int		i;

//
// figure button bits
//	

	if ( in_attack.state & 3 )
		cmd->buttons |= BUTTON_ATTACK;
	in_attack.state &= ~2;
	
	if (in_use.state & 3)
		cmd->buttons |= BUTTON_USE;
	in_use.state &= ~2;

	if (anykeydown && cls.key_dest == key_game)
		cmd->buttons |= BUTTON_ANY;

	// send milliseconds of time to apply the move
	ms = (int)(cls.frametime * 1000);
	if (ms > 250)
		ms = 100;		// time was unreasonable
	cmd->msec = (byte)ms;

	CL_ClampPitch ();
	for (i=0 ; i<3 ; i++)
		cmd->angles[i] = ANGLE2SHORT(cl.viewangles[i]);

	cmd->impulse = (byte)in_impulse;
	in_impulse = 0;

	//update (7588+), we use 400 since water move physics is "buggy" in that it uses uncapped usercmd
	//values, but it's been gameplay since q2 began, so keep with it to be compatible.
	if (cmd->forwardmove > 400)
		cmd->forwardmove = 400;
	else if (cmd->forwardmove < -400)
		cmd->forwardmove = -400;

	if (cmd->sidemove > 400)
		cmd->sidemove = 400;
	else if (cmd->sidemove < -400)
		cmd->sidemove = -400;

	if (cmd->upmove > 400)
		cmd->upmove = 400;
	else if (cmd->upmove < -400)
		cmd->upmove = -400;

// send the ambient light level at the player's current position
	cmd->lightlevel = (byte)cl_lightlevel->value;
}

usercmd_t CL_CreateCmd (void)
{
	usercmd_t	cmd;

	frame_msec = sys_frame_time - old_sys_frame_time;

	if (frame_msec < 1)
		frame_msec = 1;

	if (frame_msec > 200)
		frame_msec = 200;
	
	// get basic movement from keyboard
	CL_BaseMove_Synchronous (&cmd);

	// allow mice or other external controllers to add to the move
	IN_Move (&cmd);

	CL_FinishMove (&cmd);

	old_sys_frame_time = sys_frame_time;

//cmd.impulse = cls.framecount;

	return cmd;
}

void CL_SendCmd_Synchronous (void)
{
	sizebuf_t	buf;
	byte		data[128];
	int			i;
	usercmd_t	*cmd, *oldcmd;
	int			checksumIndex = 0;

	// build a command even if not connected

	// save this command off for prediction
	i = cls.netchan.outgoing_sequence & (CMD_BACKUP-1);
	cmd = &cl.cmds[i];
	cl.cmd_time[i] = cls.realtime;	// for netgraph ping calculation

	*cmd = CL_CreateCmd ();

	cl.cmd = *cmd;

	if (cls.state == ca_disconnected || cls.state == ca_connecting)
		return;

	if ( cls.state == ca_connected)
	{
		if (cls.netchan.got_reliable || cls.netchan.message.cursize	|| (unsigned)(curtime - cls.netchan.last_sent) > 100 ) {
	 		//memset (&buf, 0, sizeof(buf));
			//Com_DPrintf ("connected: flushing netchan (len=%d, %s)\n", cls.netchan.message.cursize, MakePrintable(cls.netchan.message.data));
			Netchan_Transmit (&cls.netchan, 0, NULL);	
		}
		return;
	}

	//FIXME: fix this and test if it works
	/*if (cl_async->intvalue == 2)
	{
		i = (cls.netchan.outgoing_sequence-1) & (CMD_BACKUP-1);
		oldcmd = &cl.cmds[i];

		if (cmd->forwardmove == oldcmd->forwardmove && cmd->sidemove == oldcmd->sidemove && cmd->upmove == oldcmd->upmove && cmd->impulse == oldcmd->impulse && cmd->buttons == oldcmd->buttons)
		{
			if (cmd->msec + oldcmd->msec < 250)
			{
				cmd->msec += oldcmd->msec;
				//oldcmd->msec = 0;
				memset (oldcmd, 0, sizeof(*oldcmd));
				return;
			}
		}
	}*/

	// send a userinfo update if needed
	if (userinfo_modified)
	{
		CL_FixUpGender();
		userinfo_modified = false;
		MSG_WriteByte (clc_userinfo);
		MSG_WriteString (Cvar_Userinfo());
		MSG_EndWriting (&cls.netchan.message);
	}

	SZ_Init (&buf, data, sizeof(data));

#ifdef CINEMATICS
	if (cmd->buttons && cl.cinematictime > 0 && !cl.attractloop 
		&& cls.realtime - cl.cinematictime > 1000)
	{	// skip the rest of the cinematic
		SCR_FinishCinematic ();
	}
#endif

	// begin a client move command
	MSG_WriteByte (clc_move);

	// save the position for a checksum byte
	// save the position for a checksum byte
	if (cls.serverProtocol == PROTOCOL_ORIGINAL)
	{
		checksumIndex = MSG_GetLength();// buf.cursize;
		MSG_WriteByte (0);
	}

	// let the server know what the last frame we
	// got was, so the next message can be delta compressed
	if (cl_nodelta->value || !cl.frame.valid || cls.demowaiting)
		MSG_WriteLong (-1);	// no compression
	else
		MSG_WriteLong (cl.frame.serverframe);

	// send this and the previous cmds in the message, so
	// if the last packet was dropped, it can be recovered
	i = (cls.netchan.outgoing_sequence-2) & (CMD_BACKUP-1);
	cmd = &cl.cmds[i];

	MSG_WriteDeltaUsercmd (&null_usercmd, cmd, cls.protocolVersion);
	oldcmd = cmd;

	i = (cls.netchan.outgoing_sequence-1) & (CMD_BACKUP-1);
	cmd = &cl.cmds[i];
	MSG_WriteDeltaUsercmd (oldcmd, cmd, cls.protocolVersion);
	oldcmd = cmd;

	i = (cls.netchan.outgoing_sequence) & (CMD_BACKUP-1);
	cmd = &cl.cmds[i];
	MSG_WriteDeltaUsercmd (oldcmd, cmd, cls.protocolVersion);

	MSG_EndWriting (&buf);

	// calculate a checksum over the move commands
	if (cls.serverProtocol == PROTOCOL_ORIGINAL)
	{
		buf.data[checksumIndex] = COM_BlockSequenceCRCByte(
			buf.data + checksumIndex + 1, buf.cursize - checksumIndex - 1,
			cls.netchan.outgoing_sequence);
	}

	//
	// deliver the message
	//
	Netchan_Transmit (&cls.netchan, buf.cursize, buf.data);	
}

/*
=================
CL_SendCmd
=================
*/
void CL_SendCmd (void)
{
	sizebuf_t	buf;
	byte		data[128];
	int			i;
	usercmd_t	*cmd, *oldcmd;
	int			checksumIndex = 0;

	if (cls.state <= ca_connecting)
		return;

	if ( cls.state == ca_connected)
	{
		if (cls.netchan.got_reliable || cls.netchan.message.cursize	|| (unsigned)(curtime - cls.netchan.last_sent) > 100 ) {
	 		//memset (&buf, 0, sizeof(buf));
			//Com_DPrintf ("connected: flushing netchan (len=%d, %s)\n", cls.netchan.message.cursize, MakePrintable(cls.netchan.message.data));
			Netchan_Transmit (&cls.netchan, 0, NULL);	
		}
		return;
	}

	i = cls.netchan.outgoing_sequence & (CMD_BACKUP-1);
	cmd = &cl.cmds[i];
	cl.cmd_time[i] = cls.realtime;	// for netgraph ping calculation

	//jec - prepare the pending user command for sending.
	CL_FinalizeCmd();

	cl.cmd = *cmd;

	// send a userinfo update if needed
	if (userinfo_modified)
	{
		CL_FixUpGender();
		userinfo_modified = false;
		MSG_WriteByte (clc_userinfo);
		MSG_WriteString (Cvar_Userinfo());
		MSG_EndWriting (&cls.netchan.message);
	}

	SZ_Init (&buf, data, sizeof(data));

#ifdef CINEMATICS
	if (cmd->buttons && cl.cinematictime > 0 && !cl.attractloop 
		&& cls.realtime - cl.cinematictime > 1000)
	{	// skip the rest of the cinematic
		SCR_FinishCinematic ();
	}
#endif

	MSG_WriteByte (clc_move);

	// save the position for a checksum byte
	if (cls.serverProtocol == PROTOCOL_ORIGINAL)
	{
		checksumIndex = MSG_GetLength();// buf.cursize;
		MSG_WriteByte (0);
	}

	// let the server know what the last frame we
	// got was, so the next message can be delta compressed

	//r1: after a vid_restart memory locations of models changes! all existing ents
	//need to be re-sent so the client updates its model to the new memory location.
	if (!cl.frame.valid || cls.demowaiting || cl_nodelta->intvalue)
		MSG_WriteLong (-1);	// no compression
	else
		MSG_WriteLong (cl.frame.serverframe);

	// send this and the previous cmds in the message, so
	// if the last packet was dropped, it can be recovered
	i = (cls.netchan.outgoing_sequence-2) & (CMD_BACKUP-1);
	cmd = &cl.cmds[i];
	//memset (&nullcmd, 0, sizeof(nullcmd));
	MSG_WriteDeltaUsercmd (&null_usercmd, cmd, cls.protocolVersion);
	oldcmd = cmd;

	i = (cls.netchan.outgoing_sequence-1) & (CMD_BACKUP-1);
	cmd = &cl.cmds[i];
	MSG_WriteDeltaUsercmd (oldcmd, cmd, cls.protocolVersion);
	oldcmd = cmd;

	i = (cls.netchan.outgoing_sequence) & (CMD_BACKUP-1);
	cmd = &cl.cmds[i];
	MSG_WriteDeltaUsercmd (oldcmd, cmd, cls.protocolVersion);
	MSG_EndWriting (&buf);

	// calculate a checksum over the move commands
	if (cls.serverProtocol == PROTOCOL_ORIGINAL)
	{
		buf.data[checksumIndex] = COM_BlockSequenceCRCByte(
			buf.data + checksumIndex + 1, buf.cursize - checksumIndex - 1,
			cls.netchan.outgoing_sequence);
	}

	//
	// deliver the message
	//

	Netchan_Transmit (&cls.netchan, buf.cursize, buf.data);	

	CL_InitCmd(); //jec - init the next usercmd buffer.
}


