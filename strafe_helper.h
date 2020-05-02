
#include <math.h>
//#include <stdbool.h>
#include "client/client.h"


typedef struct {
	qboolean center;
	qboolean center_marker;
	float scale;
	float height;
	float y;
	int32 colorcenter;
	int32 coloraccel;
	int32 coloroptimal;
}struct_params;

void StrafeHelper_SetAccelerationValues(const float forward[3],
	const float velocity[3],
	const float wishdir[3],
	const float wishspeed,
	const float accel,
	const float frametime);


void StrafeHelper_Draw(struct_params params, const float hud_width, const float hud_height);

/*
int32 shi_color_center_marker = 3;
int32 shi_color_accelerating = 197;
int32 shi_color_optimal = 193;
*/


static inline void shi_drawFilledRectangle(const float x, const float y,
	const float w, const float h,
	const int32 color)
{
	re.DrawFill((int)x, (int)y, (int)w, (int)h, color);
}

struct_params params;