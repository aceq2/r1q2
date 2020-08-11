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
// client.h -- primary header for client

//define	PARANOID			// speed sapping error checking
#ifndef _CLIENT_H

#define	BUILDING_CLIENT		1
#define	DECOUPLED_RENDERER	1

//shared by keys/console
#define	MAXCMDLINE	512

#include <math.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <gl/GL.h>
#include <curl.h>

#ifdef USE_CURL
#ifdef _WIN32
#define CURL_STATICLIB
#define CURL_HIDDEN_SYMBOLS
#define CURL_EXTERN_SYMBOL
#define CURL_CALLING_CONVENTION __cdecl
#endif
#include <../curl/curl.h>
#endif

#include "ref.h"

#include "vid.h"
#include "screen.h"
#include "sound.h"
#include "input.h"
#include "keys.h"
#include "console.h"
#include "cdaudio.h"

//=============================================================================

typedef struct
{
	qboolean		valid;			// cleared if delta parsing was invalid
	int				serverframe;
	int				servertime;		// server time the message is valid for (in msec)
	int				deltaframe;
	byte			areabits[MAX_MAP_AREAS/8];		// portalarea visibility bits
	player_state_t	playerstate;
	int				num_entities;
	int				parse_entities;	// non-masked index into cl_parse_entities array
} frame_t;

typedef struct
{
	entity_state_t	baseline;//[UPDATE_BACKUP];		// delta from this if not from a previous frame
	entity_state_t	current;
	entity_state_t	prev;			// will always be valid, but might just be a copy of current

	int			serverframe;		// if not current, this ent isn't in the frame

	int			trailcount;			// for diminishing grenade trails
	vec3_t		lerp_origin;		// for trails (variable hz)

	int			fly_stoptime;

	int			frame_lerp_from;
	int			frame_lerp_to;
	int			lerp_time;
} centity_t;

#define MAX_CLIENTWEAPONMODELS		20		// PGM -- upped from 16 to fit the chainfist vwep

typedef struct
{
	char			name[MAX_QPATH];
	char			cinfo[MAX_QPATH];
	struct image_s	*skin;
	struct image_s	*icon;
	char			iconname[MAX_QPATH];
	struct model_s	*model;
	struct model_s	*weaponmodel[MAX_CLIENTWEAPONMODELS];
	qboolean		deferred;
} clientinfo_t;

extern char cl_weaponmodels[MAX_CLIENTWEAPONMODELS][MAX_QPATH];
extern int num_cl_weaponmodels;

#define	CMD_BACKUP		64	// allow a lot of command backups for very fast systems

//r1: localents
typedef struct localent_s localent_t;

struct localent_s
{
	//complete entity
	entity_t	ent;

	char		*classname;

	//in use?
	qboolean	inuse;

	int			movetype;
	
	vec3_t		mins;
	vec3_t		maxs;
	vec3_t		velocity;

	//we want local ents to be able to pseudo-think too (for freeing, effects, et al)
	//FIXME BIG HACK OF DOOM:
	//local ents run physics from gloom code for thinking, etc.
	float		nextthink;
	void		(*think)(localent_t *self);
	void		(*touch)(localent_t *self, cplane_t *plane, csurface_t *surf);
};

//yummmm splash gibs death blood
#define MAX_LOCAL_ENTS 1024

localent_t cl_localents[MAX_LOCAL_ENTS];

void Le_Reset (void);

#ifdef USE_CURL
void CL_CancelHTTPDownloads (qboolean permKill);
void CL_InitHTTPDownloads (void);
qboolean CL_QueueHTTPDownload (const char *quakePath);
void CL_RunHTTPDownloads (void);
qboolean CL_PendingHTTPDownloads (void);
void CL_SetHTTPServer (const char *URL);
void CL_HTTP_Cleanup (qboolean fullShutdown);

typedef enum
{
	DLQ_STATE_NOT_STARTED,
	DLQ_STATE_RUNNING,
	DLQ_STATE_DONE
} dlq_state;

typedef struct dlqueue_s
{
	struct dlqueue_s	*next;
	char				quakePath[MAX_QPATH];
	dlq_state			state;
} dlqueue_t;

typedef struct dlhandle_s
{
	CURL		*curl;
	char		filePath[MAX_OSPATH];
	FILE		*file;
	dlqueue_t	*queueEntry;
	size_t		fileSize;
	size_t		position;
	double		speed;
	char		URL[576];
	char		*tempBuffer;
} dlhandle_t;
#endif

/*typedef struct
{
	char		name[MAX_QPATH];
	int			download_attempted;
} configstring_t;*/

//
// the client_state_t structure is wiped completely at every
// server map change
//

typedef struct client_state_s
{
	int			timeoutcount;

	int			timedemo_frames;
	unsigned 	timedemo_start;

	qboolean	refresh_prepped;	// false if on new level or new ref dll
	qboolean	sound_prepped;		// ambient sounds can start
	qboolean	force_refdef;		// vid has changed, so we can't use a paused refdef

	int			parse_entities;		// index (not anded off) into cl_parse_entities[]

	usercmd_t	cmd;
	usercmd_t	cmds[CMD_BACKUP];	// each mesage will send several old cmds
	int			cmd_time[CMD_BACKUP];	// time sent, for calculating pings
	int16		predicted_origins[CMD_BACKUP][3];	// for debug comparing against server

	float		predicted_step;				// for stair up smoothing
	unsigned	predicted_step_time;

	vec3_t		predicted_origin;	// generated by CL_PredictMovement
	vec3_t		predicted_angles;
	vec3_t		prediction_error;

	frame_t		frame;				// received from server
	int			surpressCount;		// number of messages rate supressed
	frame_t		frames[UPDATE_BACKUP];

	// the client maintains its own idea of view angles, which are
	// sent to the server each frame.  It is cleared to 0 upon entering each level.
	// the server sends a delta each frame which is added to the locally
	// tracked view angles to account for standing on rotating objects,
	// and teleport direction changes
	vec3_t		viewangles;

	int			time;			// this is the time value that the client
	int			initial_server_frame;
								// is rendering at.  always <= cls.realtime
	float		lerpfrac;		// between oldframe and frame

	refdef_t	refdef;

	vec3_t		v_forward, v_right, v_up;	// set when refdef.angles is set

	//
	// transient data from server
	//
	char		layout[1024];		// general 2D overlay
	int			inventory[MAX_ITEMS];

	//
	// non-gameserver infornamtion
	// FIXME: move this cinematic stuff into the cin_t structure
#ifdef CINEMATICS
	FILE		*cinematic_file;
	int			cinematictime;		// cls.realtime for first cinematic frame
	int			cinematicframe;
	char		cinematicpalette[768];
	qboolean	cinematicpalette_active;
#endif

	//
	// server state information
	//
	qboolean	attractloop;		// running the attract loop, any key will menu
	int			servercount;	// server identification for prespawns
	char		gamedir[MAX_QPATH];
	int			playernum;
	int			maxclients;

	char		configstrings[MAX_CONFIGSTRINGS][MAX_QPATH];

	//
	// locally derived information from server state
	//
	struct model_s	*model_draw[MAX_MODELS];
	struct cmodel_s	*model_clip[MAX_MODELS];

	struct sfx_s	*sound_precache[MAX_SOUNDS];
	//struct image_s	*image_precache[MAX_IMAGES];

	clientinfo_t	clientinfo[MAX_CLIENTS];
	clientinfo_t	baseclientinfo;

	qboolean		enhancedServer;
	qboolean		strafeHack;

	//r1: defer rendering when realtime < this
	uint32			defer_rendering;

	byte			demoFrame[1400];
	sizebuf_t		demoBuff;
	frame_t			*demoLastFrame;

	unsigned		settings[SVSET_MAX];
	int				player_update_time;
	float			playerlerp;
	float			modelfrac;
	
	//variable FPS support variables
	int				gunlerp_start, gunlerp_end, gunlerp_frame_from, gunlerp_frame_to;

	int				kicklerp_end;
	vec3_t			kicklerp_from, kicklerp_to;
} client_state_t;

extern	client_state_t	cl;

/*
==================================================================

the client_static_t structure is persistant through an arbitrary number
of server connections

==================================================================
*/

typedef enum {
	ca_uninitialized,
	ca_disconnected, 	// not talking to a server
	ca_connecting,		// sending request packets to the server
	ca_connected,		// netchan_t established, waiting for svc_serverdata
	ca_active			// game views should be displayed
} connstate_t;

/*typedef enum {
	dl_none,
	dl_model,
	dl_sound,
	dl_skin,
	dl_single
} dltype_t;		// download type*/

typedef enum {
	ps_none,
	ps_pending,
	ps_active
} proxystate_t;

typedef enum {key_game, key_console, key_message, key_menu} keydest_t;

typedef struct client_static_s
{
	connstate_t	state;
	keydest_t	key_dest;

	//unsigned int	framecount;
	uint32			lastSpamTime;
	uint32			spamTime;
	uint32			realtime;			// always increasing, no clamping, etc
	float			frametime;			// seconds since last frame

// screen rendering information
	uint32			disable_screen;		// showing loading plaque between levels
									// or changing rendering dlls
									// if time gets > 30 seconds ahead, break it
	int				disable_servercount;	// when we receive a frame and cl.servercount
									// > cls.disable_servercount, clear disable_screen

// connection information
	char			servername[MAX_OSPATH];	// name of server from original connect
	char			lastservername[MAX_OSPATH];	// name of server from original connect
	int				connect_time;		// for connection retransmits

	int			quakePort;			// a 16 bit value that allows quake servers
									// to work around address translating routers
	netchan_t	netchan;
	int			serverProtocol;		// in case we are doing some kind of version hack

	int			challenge;			// from the server to use for connecting

	FILE		*download;			// file transfer from server
	char		downloadtempname[MAX_OSPATH];
	char		downloadname[MAX_OSPATH];
	qboolean	downloadpending;
	qboolean	failed_download;
	//dltype_t	downloadtype;
	int			downloadsize;
	size_t		downloadposition;
	int			downloadpercent;

// demo recording info must be here, so it isn't cleared on level change
	qboolean	demorecording;
	qboolean	demowaiting;	// don't record until a non-delta message is received
	qboolean	passivemode;
	FILE		*demofile;

	int			protocolVersion;	// R1Q2 protocol version

#ifdef USE_CURL
	dlqueue_t		downloadQueue;			//queue of paths we need
	
	dlhandle_t		HTTPHandles[4];			//actual download handles
	//don't raise this!
	//i use a hardcoded maximum of 4 simultaneous connections to avoid
	//overloading the server. i'm all too familiar with assholes who set
	//their IE or Firefox max connections to 16 and rape my Apache processes
	//every time they load a page... i'd rather not have my q2 client also
	//have the ability to do so - especially since we're possibly downloading
	//large files.

	char			downloadServer[512];	//base url prefix to download from
	char			downloadReferer[32];	//libcurl requires a static string :(
#endif

	char			followHost[32];
	proxystate_t	proxyState;
	netadr_t		proxyAddr;
} client_static_t;

extern client_static_t	cls;

//=============================================================================

//
// cvars
//
#ifdef CL_STEREO_SUPPORT
extern	cvar_t	*cl_stereo_separation;
extern	cvar_t	*cl_stereo;
#endif

extern	cvar_t	*cl_gun;
extern	cvar_t	*cl_add_blend;
extern	cvar_t	*cl_add_lights;
extern	cvar_t	*cl_add_particles;
extern	cvar_t	*cl_add_entities;
extern	cvar_t	*cl_predict;
extern	cvar_t	*cl_backlerp;
extern	cvar_t	*cl_footsteps;
extern	cvar_t	*cl_smoothsteps;
extern	cvar_t	*cl_noskins;
//extern	cvar_t	*cl_autoskins;

extern	cvar_t	*cl_upspeed;
extern	cvar_t	*cl_forwardspeed;
extern	cvar_t	*cl_sidespeed;

extern	cvar_t	*cl_yawspeed;
extern	cvar_t	*cl_pitchspeed;

extern	cvar_t	*cl_run;

extern	cvar_t	*cl_anglespeedkey;

extern	cvar_t	*cl_shownet;
extern	cvar_t	*cl_showmiss;
extern	cvar_t	*cl_showclamp;

extern	cvar_t	*cl_filterchat;

extern	cvar_t	*lookspring;
extern	cvar_t	*lookstrafe;
extern	cvar_t	*sensitivity;

extern	cvar_t	*m_pitch;
extern	cvar_t	*m_yaw;
extern	cvar_t	*m_forward;
extern	cvar_t	*m_side;

extern	cvar_t	*freelook;

extern	cvar_t	*cl_lightlevel;	// FIXME HACK

extern	cvar_t	*cl_paused;
extern	cvar_t	*cl_timedemo;

extern	cvar_t	*cl_vwep;

//extern	cvar_t	*cl_defertimer;

extern	cvar_t	*scr_sizegraph;
extern	cvar_t	*fs_gamedirvar;

extern	cvar_t	*cl_nolerp;
//extern	cvar_t	*cl_snaps;

extern	cvar_t	*cl_railtrail;
extern	cvar_t	*cl_async;


//r1q2 speed-hud
extern cvar_t   *cl_drawStrafeHelper;
extern cvar_t   *cl_strafeHelperCenter;
extern cvar_t   *cl_strafeHelperCenterMarker;
extern cvar_t   *cl_strafeHelperHeight;
extern cvar_t   *cl_strafeHelperScale;
extern cvar_t   *cl_strafeHelperY;
extern cvar_t	*cl_strafeColorCenter;
extern cvar_t	*cl_strafeColorAccel;
extern cvar_t	*cl_strafeColorOptimal;


extern	cvar_t	*cl_protocol;
extern	cvar_t	*cl_test;
extern	cvar_t	*cl_test2;
extern	cvar_t	*cl_test3;

extern cvar_t	*mset_fog;
extern cvar_t	*gl_fog;
extern cvar_t	*gl_fog_r;
extern cvar_t	*gl_fog_g;
extern cvar_t	*gl_fog_b;
extern cvar_t	*gl_fog_a;
extern cvar_t	*gl_fog_density;
extern cvar_t	*gl_fogstart;
extern cvar_t	*gl_fogend;

#ifdef USE_CURL
extern	cvar_t	*cl_http_downloads;
extern	cvar_t	*cl_http_filelists;
extern	cvar_t	*cl_http_proxy;
extern	cvar_t	*cl_http_max_connections;
#endif

extern	cvar_t	*cl_original_dlights;
extern	cvar_t	*cl_default_location;
extern	cvar_t	*cl_player_updates;

extern	cvar_t	*fov;

extern	cvar_t	*vid_fullscreen;

extern	cvar_t	*cl_quietstartup;

#ifndef DEDICATED_ONLY
extern	qboolean send_packet_now;
#endif

typedef struct
{
	int			entity;				// so entities can reuse same entry
	int			die;				// stop lighting after this time

	qboolean	follow;			// r1: follow entity

	vec3_t		color;
	vec3_t		origin;

	float		radius;
	//float	minlight;			// don't add when contributing less
} cdlight_t;

extern	centity_t	cl_entities[MAX_EDICTS];
extern	cdlight_t	cl_dlights[MAX_DLIGHTS];

// the cl_parse_entities must be large enough to hold UPDATE_BACKUP frames of
// entities, so that when a delta compressed message arives from the server
// it can be un-deltad from the original 
#define	MAX_PARSE_ENTITIES	1024
extern	entity_state_t	cl_parse_entities[MAX_PARSE_ENTITIES];

//=============================================================================

extern	qboolean os_winxp;

extern	netadr_t	net_from;
extern	sizebuf_t	net_message;

void DrawString (int x, int y, const char *s);
void DrawAltString (int x, int y, const char *s);	// toggle high bit
qboolean	CL_CheckOrDownloadFile (const char *filename);

void CL_AddNetgraph (void);
void CL_AddSizegraph (void);

//ROGUE
typedef struct cl_sustain
{
	int			id;
	int			type;
	int			endtime;
	int			nextthink;
	int			thinkinterval;
	vec3_t		org;
	vec3_t		dir;
	int			color;
	int			count;
	int			magnitude;
	void		(*think)(struct cl_sustain *self);
} cl_sustain_t;

#define MAX_SUSTAINS		32
void CL_ParticleSteamEffect2(cl_sustain_t *self);

void CL_TeleporterParticles (entity_state_t *ent);
void CL_ParticleEffect (vec3_t org, vec3_t dir, int color, int count);
void CL_ParticleEffect2 (vec3_t org, vec3_t dir, int color, int count);

// RAFAEL
void CL_ParticleEffect3 (vec3_t org, vec3_t dir, int color, int count);


qboolean CL_IgnoreMatch (const char *string);

//=================================================

// ========
// PGM
typedef struct particle_s
{
	struct particle_s	*next;

	int			type;
	int			color;

	vec3_t		org;
	vec3_t		vel;
	vec3_t		accel;

	//float		colorvel;
	float		time;
	float		alpha;
	float		alphavel;
} cparticle_t;


#define	PARTICLE_GRAVITY	40
#define BLASTER_PARTICLE_COLOR		0xe0
// PMM
#define PT_NONE		0
#define PT_INSTANT	1
// PGM
// ========

void CL_ClearEffects (void);
void CL_ClearTEnts (void);
void CL_BlasterTrail (vec3_t start, vec3_t end);
void CL_QuadTrail (vec3_t start, vec3_t end);
void CL_RailTrail (vec3_t start, vec3_t end, byte clr);
void CL_BubbleTrail (vec3_t start, vec3_t end);
void CL_FlagTrail (vec3_t start, vec3_t end, int color);

// RAFAEL
void CL_IonripperTrail (vec3_t start, vec3_t end);

// ========
// PGM
void CL_BlasterParticles2 (vec3_t org, vec3_t dir, uint32 color);
void CL_BlasterTrail2 (vec3_t start, vec3_t end);
void CL_DebugTrail (vec3_t start, vec3_t end);
void CL_SmokeTrail (vec3_t start, vec3_t end, int colorStart, int colorRun, int spacing);
void CL_Flashlight (int ent, vec3_t pos);
void CL_ForceWall (vec3_t start, vec3_t end, int color);
//void CL_FlameEffects (vec3_t origin);
void CL_GenericParticleEffect (vec3_t org, vec3_t dir, int color, int count, int numcolors, int dirspread, float alphavel);
void CL_BubbleTrail2 (vec3_t start, vec3_t end, int dist);
void CL_Heatbeam (vec3_t start, vec3_t end);
void CL_ParticleSteamEffect (vec3_t org, vec3_t dir, int color, int count, int magnitude);
void CL_TrackerTrail (vec3_t start, vec3_t end, int particleColor);
void CL_Tracker_Explode(vec3_t origin);
void CL_TagTrail (vec3_t start, vec3_t end, int color);
void CL_ColorFlash (vec3_t pos, int ent, float intensity, float r, float g, float b);
void CL_Tracker_Shell(vec3_t origin);
void CL_MonsterPlasma_Shell(vec3_t origin);
void CL_ColorExplosionParticles (vec3_t org, int color, int run);
void CL_ParticleSmokeEffect (vec3_t org, vec3_t dir, int color, int count, int magnitude);
void CL_Widowbeamout (cl_sustain_t *self);
void CL_Nukeblast (cl_sustain_t *self);
void CL_WidowSplash (vec3_t org);
// PGM
// ========

//r1ch:localents
localent_t *Le_Alloc (void);
void Le_Free (localent_t *lent);

int CL_ParseEntityBits (uint32 *bits);
void CL_ParseDelta (const entity_state_t *from, entity_state_t *to, int number, int bits);
void CL_ParseFrame (int extrabits);

void CL_ParseTEnt (void);
void CL_ParseConfigString (void);
void CL_ParseMuzzleFlash (void);
void CL_ParseMuzzleFlash2 (void);
void SmokeAndFlash(vec3_t origin);

void CL_SetLightstyle (int i);

//void CL_RunParticles (void);
void CL_RunDLights (void);
void CL_RunLightStyles (void);

void CL_AddEntities (void);
void CL_AddDLights (void);
void CL_AddTEnts (void);
void CL_AddLightStyles (void);

//=================================================

void CL_PrepRefresh (void);
void CL_RegisterSounds (void);

NORETURN void CL_Quit_f (void);

void IN_Accumulate (void);

void VID_ReloadRefresh (void);

//void CL_ParseLayout (void);


//
// cl_main
//
extern	refexport_t	re;		// interface to refresh .dll

void CL_Init (void);

void CL_FixUpGender(void);
void CL_Disconnect (qboolean skipdisconnect);
void CL_Disconnect_f (void);
void CL_GetChallengePacket (void);
void CL_PingServers_f (void);
void CL_Snd_Restart_f (void);
void CL_RequestNextDownload (void);

#ifdef CLIENT_DLL
void CL_ClDLL_Restart_f (void);
#endif

//
// cl_input
//
typedef struct
{
	int32	down[2];		// key nums holding it down
	uint32	downtime;		// msec timestamp
	uint32	msec;			// msec down this frame
	int32	state;
} kbutton_t;

extern	kbutton_t	in_mlook, in_klook;
extern 	kbutton_t 	in_strafe;
extern 	kbutton_t 	in_speed;

void CL_InitInput (void);
void CL_SendCmd (void);
void CL_SendCmd_Synchronous (void);
void CL_SendMove (usercmd_t *cmd);

void CL_ClearState (void);

void CL_ReadPackets (void);

int  CL_ReadFromServer (void);
void CL_WriteToServer (usercmd_t *cmd);
void CL_BaseMove (usercmd_t *cmd);

void IN_CenterView (void);

float CL_KeyState (kbutton_t *key);
char *Key_KeynumToString (int keynum);

//
// cl_demo.c
//
void CL_WriteFullDemoMessage (void);
void CL_WriteDemoMessage (byte *buff, int len, qboolean forceFlush);
void CL_Stop_f (void);
void CL_Record_f (void);

//
// cl_parse.c
//
extern	int	serverPacketCount;
extern	int noFrameFromServerPacket;

qboolean CL_ParseServerMessage (void);
void CL_LoadClientinfo (clientinfo_t *ci, char *s);
void SHOWNET(const char *s);
void CL_ParseClientinfo (int player);
void CL_Download_f (void);
void CL_Passive_f (void);
void CL_ParsePlayerUpdate (void);
//
// cl_view.c
//
extern	int			gun_frame;
extern	struct model_s	*gun_model;

void V_Init (void);
void Set_Fog (void);
#ifdef CL_STEREO_SUPPORT
void V_RenderView( float stereo_separation );
#else
void V_RenderView( void );
#endif
void V_AddEntity (entity_t *ent);
void V_AddParticle (vec3_t org, unsigned color, float alpha);
void V_AddLight (vec3_t org, float intensity, float r, float g, float b);
void V_AddLightStyle (int style, float r, float g, float b);

void IN_DeactivateMouse (void);

//
// cl_tent.c
//
void CL_RegisterTEntSounds (void);
void CL_RegisterTEntModels (void);
void CL_SmokeAndFlash(vec3_t origin);


//
// cl_pred.c
//
void CL_InitPrediction (void);
void CL_PredictMove (void);
void CL_CheckPredictionError (void);

//
// cl_fx.c
//
cdlight_t *CL_AllocDlight (int entity, qboolean follow);
void CL_BigTeleportParticles (vec3_t org);
void CL_RocketTrail (vec3_t start, vec3_t end, centity_t *old);
void CL_DiminishingTrail (vec3_t start, vec3_t end, centity_t *old, int flags);
void CL_FlyEffect (centity_t *ent, vec3_t origin);
void CL_BfgParticles (entity_t *ent);
void CL_AddParticles (void);
void CL_EntityEvent (entity_state_t *ent);
// RAFAEL
void CL_TrapParticles (entity_t *ent);

//
// menus
//
void M_Init (void);
void M_Keydown (int key);
void M_Draw (void);
void M_Menu_Main_f (void);
void M_ForceMenuOff (void);
void M_AddToServerList (netadr_t adr, char *info);

//
// cl_inv.c
//
void CL_ParseInventory (void);
void CL_KeyInventory (int key);
void CL_DrawInventory (void);

void IN_Restart_f (void);

//
// cl_pred.c
//
void CL_PredictMovement (void);

void CL_FixCvarCheats (void);

//
// keys.c
//
int				match_found;
int				cmdnr;
const char		*matchingcmds;

#ifdef CLIENT_DLL
typedef struct
{
	// if api_version is different, the dll cannot be used
	int		api_version;

	// called when the library is loaded
	int		(IMPORT *Init) ( void *hinstance );

	// called before the library is unloaded
	void	(IMPORT *Shutdown) (void);

	// exported functions
	qboolean	(IMPORT *CLParseTempEnt) (int type);
} clexport_t;


typedef struct
{
	//network reading functions
	int		(EXPORT *MSG_ReadChar) (sizebuf_t *sb);
	int		(EXPORT *MSG_ReadByte) (sizebuf_t *sb);
	int		(EXPORT *MSG_ReadShort) (sizebuf_t *sb);
	int		(EXPORT *MSG_ReadLong) (sizebuf_t *sb);
	float	(EXPORT *MSG_ReadFloat) (sizebuf_t *sb);
	char	*(EXPORT *MSG_ReadString) (sizebuf_t *sb);
	char	*(EXPORT *MSG_ReadStringLine) (sizebuf_t *sb);

	float	(EXPORT *MSG_ReadCoord) (sizebuf_t *sb);
	void	(EXPORT *MSG_ReadPos) (sizebuf_t *sb, vec3_t pos);
	float	(EXPORT *MSG_ReadAngle) (sizebuf_t *sb);
	float	(EXPORT *MSG_ReadAngle16) (sizebuf_t *sb);

	void	(EXPORT *MSG_ReadDir) (sizebuf_t *sb, vec3_t vector);
	void	(EXPORT *MSG_ReadData) (sizebuf_t *sb, void *buffer, int size);

	void	(EXPORT *Cmd_AddCommand) (const char *name, void(*cmd)(void));
	void	(EXPORT *Cmd_RemoveCommand) (const char *name);

	int		(EXPORT *Cmd_Argc) (void);
	char	*(EXPORT *Cmd_Argv) (int i);

	void	(EXPORT *Cmd_ExecuteText) (int exec_when, char *text);

	void	(EXPORT *Com_Error) (int err_level, const char *str, ...);
	void	(EXPORT *Com_Printf) (const char *str, ...);

	// files will be memory mapped read only
	// the returned buffer may be part of a larger pak file,
	// or a discrete file from anywhere in the quake search path
	// a -1 return means the file does not exist
	// NULL can be passed for buf to just determine existance
	int		(EXPORT *FS_LoadFile) (const char *name, void **buf);
	void	(EXPORT *FS_FreeFile) (void *buf);

	// gamedir will be the current directory that generated
	// files should be stored to, ie: "f:\quake\id1"
	char	*(EXPORT *FS_Gamedir) (void);

	cvar_t	*(EXPORT *Cvar_Get) (const char *name, const char *value, int flags);
	cvar_t	*(EXPORT *Cvar_Set)( const char *name, const char *value );
	void	 (EXPORT *Cvar_SetValue)( const char *name, float value );

	// memory management
	void	*(EXPORT *Z_Alloc) (int size);
	void	(EXPORT *Z_Free) (void *buf);
} climport_t;


// this is the only function actually exported at the linker level
typedef	clexport_t	(EXPORT *GetClAPI_t) (climport_t);

extern	clexport_t ce;
extern	qboolean cllib_active;
#endif

#define _CLIENT_H

#endif
