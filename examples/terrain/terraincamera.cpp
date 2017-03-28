#include "stdafx.h"
#include "terraincamera.h"
#include <dukat/gamebase.h>
#include <dukat/devicemanager.h>
#include <dukat/inputdevice.h>
#include <dukat/quaternion.h>

namespace dukat
{
	TerrainCamera::TerrainCamera(GameBase* game, const Vector3& target, float distance, float longitude, float latitude)
		: Camera3(game->get_window()), game(game), look_at(target), distance(distance), min_distance(0.0f), max_distance(1000.0f),
		longitude(longitude), latitude(latitude)
	{
	}

	void TerrainCamera::update(float delta)
	{
		auto dev = game->get_devices()->active;
		if (dev->is_pressed(InputDevice::VirtualButton::SecondaryAction))
		{
			longitude -= dev->rx * delta;
			latitude += dev->ry * delta;
			clamp(latitude, -pi_over_two, pi_over_two);
		}

		Quaternion qy, qx;
		qy.set_to_rotate_y(longitude);
		qx.set_to_rotate_x(latitude);
		Matrix4 mat_rot;
		mat_rot.setup_rotation(qx * qy);
		mat_rot.extract_rotation_axis(transform.right, transform.up, transform.dir);
		transform.right = -transform.right;

		transform.position = look_at - transform.dir * distance;

		Camera3::update(delta);
	}
}