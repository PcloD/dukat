#include "stdafx.h"
#include "camera3.h"
#include "ray3.h"
#include "mathutil.h"

namespace dukat
{
	const float Camera3::default_fov = 45.0f;
	const float Camera3::default_near_clip = 1.0f;
	const float Camera3::default_far_clip = 100.0f;

	Camera3::Camera3(Window* window) : window(window), field_of_view(default_fov), 
		near_clip(default_near_clip), far_clip(default_far_clip)
	{
		window->bind(this);
	}

	Camera3::~Camera3(void)
	{
		window->unbind(this);
	}

	void Camera3::resize(int width, int height)
	{
		aspect_ratio = (float)width / (float)height;
		transform.mat_proj_pers.setup_perspective(field_of_view, aspect_ratio, near_clip, far_clip);
		transform.mat_proj_orth.setup_orthographic(-aspect_ratio, 1.0f, aspect_ratio, -1.0f, -1.0f, 1.0f);
	}

	void Camera3::update(float delta)
	{
		Vector3& p = transform.position;
		Matrix4& m = transform.mat_view;
		m[0 ] = transform.right.x; m[ 1] = transform.up.x; m[ 2] = -transform.dir.x; m[ 3] = 0.0f;
		m[4 ] = transform.right.y; m[ 5] = transform.up.y; m[ 6] = -transform.dir.y; m[ 7] = 0.0f;
		m[8 ] = transform.right.z; m[ 9] = transform.up.z; m[10] = -transform.dir.z; m[11] = 0.0f;		
		m[12] = -(transform.right * p); m[13] = -(transform.up * p); m[14] = (transform.dir * p); m[15] = 1.0f;

		// build inverse:
		//				(I|C)(R|0)      (R|0)      (I|C)   (RT|0)   (I|-C)   (RT|-RTC)
		// inv(V) = inv	(-+-)(-+-) = inv(-+-) * inv(-+-) = (--+-) * (-+--) = (--+----)
		//				(0|1)(0|1)      (0|1)      (0|1)   ( 0|1)   (0| 1)   ( 0|   1)
		Matrix4& mi = transform.mat_view_inv;
		// RT
		mi[0] = m[0]; mi[1] = m[4]; mi[2] = m[8]; mi[3] = 0.0f;
		mi[4] = m[1]; mi[5] = m[5]; mi[6] = m[9]; mi[7] = 0.0f;
		mi[8] = m[2]; mi[9] = m[6]; mi[10] = m[10]; mi[11] = 0.0f;
		// -RTC
		mi[12] = -(mi[0] * m[12] + mi[4] * m[13] + mi[8] * m[14]);
		mi[13] = -(mi[1] * m[12] + mi[5] * m[13] + mi[9] * m[14]);
		mi[14] = -(mi[2] * m[12] + mi[6] * m[13] + mi[10] * m[14]);
		mi[15] = 1.0f;
	}

	Ray3 Camera3::pick_ray_screen(float x, float y)
	{
		// TODO: implement
		//float aspect = mGame.window()->aspectRatio();
		return Ray3();
	}

	Ray3 Camera3::pick_ray_view(float x, float y)
	{
		float tanfov = tanf(0.5f * deg_to_rad(field_of_view));
		// compute points at near and far plane
		Vector3 p1(near_clip * tanfov * x, near_clip * tanfov * y, -near_clip),
			p2(far_clip * tanfov * x, far_clip * tanfov * y, -far_clip);
		// Transform points into world space
		p1 *= transform.mat_view_inv;
		p2 *= transform.mat_view_inv;
		Ray3 res;
		return res.from_points(p1, p2);
	}
}