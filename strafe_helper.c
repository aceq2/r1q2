#include "strafe_helper.h"

static float sign(const float value)
{
	return copysignf(1.0f, value);
}


static float vectorAngleSign(const float v[2], const float w[2])
{
	return sign(v[0] * w[1] - v[1] * w[0]);
}

static float dotProduct(const float v[2], const float w[2])
{
	float dot_product = 0.0f;
	for (int i = 0; i < 2; i += 1) {
		dot_product += v[i] * w[i];
	}
	return dot_product;
}

static float vectorNorm(const float v[2])
{
	return sqrtf(dotProduct(v, v));
}

/* The current angle between the players velocity vector and forward-looking
 * vector. */
static float angle_current;
/* The angle between the players velocity vector and forward-looking vector
 * that would have resulted in the maximum amount of acceleration for the
 * last reported acceleration values. */
static float angle_optimal;
/* The (absolute) minimum and maximum angles between the players velocity
 * vector and forward-looking vector that would give some acceleration. */
static float angle_minimum;
static float angle_maximum;


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static float angleDiffToPixelDiff(const float angle_difference, const float scale,
	const float hud_width)
{
	return angle_difference * (hud_width / 2.0f) * scale / (float)M_PI;
}

static float angleToPixel(const float angle, const float scale,
	const float hud_width)
{
	return (hud_width / 2.0f) - 0.5f +
		angleDiffToPixelDiff(angle, scale, hud_width);
}

void StrafeHelper_SetAccelerationValues(const float forward[3],
	const float velocity[3],
	const float wishdir[3],
	const float wishspeed,
	const float accel,
	const float frametime)
{
	const float v_z = velocity[2];
	const float w_z = wishdir[2];

	const float forward_norm = vectorNorm(forward);
	const float velocity_norm = vectorNorm(velocity);
	const float wishdir_norm = vectorNorm(wishdir);

	const float forward_velocity_angle = acosf(dotProduct(forward, wishdir)
		/ (forward_norm * wishdir_norm)) * vectorAngleSign(wishdir, forward);

	const float angle_sign = vectorAngleSign(wishdir, velocity);

	angle_optimal = (wishspeed * (1.0f - accel * frametime) - v_z * w_z)
		/ (velocity_norm * wishdir_norm);
	angle_optimal = acosf(angle_optimal);
	angle_optimal = angle_sign * angle_optimal - forward_velocity_angle;

	angle_minimum = (wishspeed - v_z * w_z) / (2.0f - wishdir_norm * wishdir_norm)
		* wishdir_norm / velocity_norm;
	angle_minimum = acosf(fminf(1.0f, angle_minimum));
	angle_minimum = angle_sign * angle_minimum - forward_velocity_angle;

	angle_maximum = -0.5f * accel * frametime * wishspeed * wishdir_norm
		/ velocity_norm;
	angle_maximum = acosf(angle_maximum);
	angle_maximum = angle_sign * angle_maximum - forward_velocity_angle;

	angle_current = dotProduct(velocity, forward) / (velocity_norm * forward_norm);
	angle_current = acosf(angle_current);
	angle_current = vectorAngleSign(forward, velocity) * angle_current;
}


void StrafeHelper_Draw(struct_params params, const float hud_width, const float hud_height)
{
	const float upper_y = (hud_height - params.height) / 2.0f + params.y;

	float offset = 0.0f;
	if (params.center) {
		offset = -angle_current;
	}

	float angle_x;
	float angle_width;
	if (angle_minimum < angle_maximum) {
		angle_x = angle_minimum + offset;
		angle_width = angle_maximum - angle_minimum;
	}
	else {
		angle_x = angle_maximum + offset;
		angle_width = angle_minimum - angle_maximum;
	}

	shi_drawFilledRectangle(
		angleToPixel(angle_x, params.scale, hud_width), upper_y,
		angleDiffToPixelDiff(angle_width, params.scale, hud_width),
		params.height, params.coloraccel); //shi_color_accelerating

	shi_drawFilledRectangle(
		angleToPixel(angle_optimal + offset, params.scale, hud_width) - 0.5f,
		upper_y, 2.0f, params.height, params.coloroptimal); //shi_color_optimal

	if (params.center_marker) {
		shi_drawFilledRectangle(
			angleToPixel(angle_current + offset, params.scale, hud_width) - 0.5f,
			upper_y + params.height / 2.0f, 2.0f, params.height / 2.0f,
			params.colorcenter); //shi_color_center_marker
	}
	//Com_Printf("scale:%f  height:%f  y:%f\n", LOG_CLIENT, params.scale, params.height, params.y);
}

