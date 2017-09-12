#include "stdafx.h"
#include "dialogscene.h"

namespace dukat
{
    DialogScene::DialogScene(Game3* game) : game(game)
	{
		overlay_meshes.stage = RenderStage::OVERLAY;
		overlay_meshes.visible = true;

		auto title_text = game->create_text_mesh(1.0f / 10.0f);
		title_text->transform.position = { -0.75f, 0.25f, 0.0f };
		title_text->set_text("<#red>Options screen</>");
		overlay_meshes.add_instance(std::move(title_text));

		auto title_info_text = game->create_text_mesh(1.0f / 20.0f);
		title_info_text->transform.position = { -1.0f, 0.0f, 0.0f };
		title_info_text->set_text("<#white>Press escape to return</>");
		overlay_meshes.add_instance(std::move(title_info_text));
	}

	void DialogScene::activate(void)
	{ 
		auto settings = game->get_settings();
		auto camera = std::make_unique<FixedCamera3>(game, Vector3{ 0.0f, 0.0f, 2.5f }, Vector3{ 0.0f, 0.0f, 0.0f }, Vector3::unit_y);
		camera->set_vertical_fov(settings.get_float("camera.fov"));
		camera->set_clip(settings.get_float("camera.nearclip"), settings.get_float("camera.farclip"));
		camera->refresh();
		game->get_renderer()->set_camera(std::move(camera));

		game->set_controller(this);
	}

	void DialogScene::update(float delta)
	{
		overlay_meshes.update(delta);
	}

	bool DialogScene::handle_keyboard(const SDL_Event& e)
	{
		switch (e.key.keysym.sym)
		{
        case SDLK_ESCAPE:
            game->pop_scene();
            break;
		default:
			return false;
		}
		return true;
	}

	void DialogScene::render(void)
	{
		auto renderer = game->get_renderer();
		std::vector<Mesh*> meshes;
		meshes.push_back(game->get_debug_meshes());
		meshes.push_back(&overlay_meshes);
		renderer->render(meshes);
	}
}