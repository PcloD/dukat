// collisionapp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "collisionapp.h"

namespace dukat
{
	CollisionScene::CollisionScene(Game2* game2) : Scene2(game2), animate(true), show_grid(true)
	{
		auto cm = game->add_manager<CollisionManager2>();
		cm->set_world_size(2000.0f);
		cm->set_world_depth(4);

		auto settings = game->get_settings();
		// Set up default camera centered around origin
		auto camera = std::make_unique<Camera2>(game, Vector2(window_width, window_height));
		camera->set_clip(settings.get_float("camera.nearclip"), settings.get_float("camera.farclip"));
		camera->refresh();
		game->get_renderer()->set_camera(std::move(camera));

		screen_dim = Vector2{ 0.5f * window_width, 0.5f * window_height };

		main_layer = game->get_renderer()->create_layer("main", 20.0f);
		debug_effect = static_cast<DebugEffect2*>(main_layer->add(std::make_unique<DebugEffect2>(game, 1.0f)));

		// Add cursor
		auto texture = game->get_textures()->get("cursor32.png");
		cursor = std::make_unique<Sprite>(texture);
		cursor->center = Sprite::align_left | Sprite::align_top;
		main_layer->add(cursor.get());

		// Add walls:
		auto wall = cm->create_body(false); // north
		wall->bb = AABB2{ -screen_dim, Vector2{ screen_dim.x, -screen_dim.y + 16.0f } };
		wall = cm->create_body(false); // east
		wall->bb = AABB2{ Vector2{ screen_dim.x - 16.0f, -screen_dim.y + 16.0f }, Vector2{ screen_dim.x, screen_dim.y - 16.0f } };
		wall = cm->create_body(false); // south
		wall->bb = AABB2{ Vector2{ -screen_dim.x, screen_dim.y - 16.0f }, Vector2{ screen_dim.x, screen_dim.y } };
		wall = cm->create_body(false); // west
		wall->bb = AABB2{ Vector2{ -screen_dim.x, -screen_dim.y + 16.0f }, Vector2{ -screen_dim.x + 16.0f, screen_dim.y - 16.0f } };

		// Add some objects
		for (auto i = 0; i < 50; i++)
			add_object();

		game->get_devices()->active->on_press(InputDevice::Button1, [&](void) {
			auto ctrl = game->get_devices()->active;
			auto pos = Vector2{ ctrl->rxa, ctrl->rya } - screen_dim;
			auto cm = game->get<CollisionManager2>();
			auto res = cm->get_bodies(pos);
			for (auto b : res)
			{
				logger << "Click on " << b->id << std::endl;
			}
		});

		// Set up info text
		info_text = game->create_text_mesh(16.0f);
		info_text->transform.position = Vector3(-0.5f * (float)window_width, 0.3f * (float)window_height, 0.0f);
		info_text->transform.update();
		main_layer->add(info_text.get());

		game->get<TimerManager>()->create_timer(0.25f, [&]() {
			auto cm = game->get<CollisionManager2>();
			std::stringstream ss;
			ss << "Objects: " << cm->body_count() << std::endl
				<< "Collisions: " << cm->contact_count() << std::endl
				<< "Tests: " << perfc.avg(PerformanceCounter::BB_CHECKS) << std::endl
				<< "<Space> Pause movement" << std::endl
				<< "<g> Toggle grid" << std::endl
				<< "<-,+> Remove / Add object" << std::endl;
			info_text->set_text(ss.str());
		}, true);

		// Set up debug layer
		auto debug_layer = game->get_renderer()->create_layer("debug", 1000.0f);
		debug_text = game->create_text_mesh(16.0f);
		debug_text->transform.position = Vector3(-0.5f * (float)window_width, -0.5f * (float)window_height, 0.0f);
		debug_text->transform.update();
		debug_layer->add(debug_text.get());
		debug_layer->hide();

		game->get<TimerManager>()->create_timer(1.0f, [&]() {
			std::stringstream ss;
			auto window = game->get_window();
			auto cam = game->get_renderer()->get_camera();
			ss << "WIN: " << window->get_width() << "x" << window->get_height()
				<< " VIR: " << cam->transform.dimension.x << "x" << cam->transform.dimension.y
				<< " FPS: " << game->get_fps()
				<< " MESH: " << perfc.avg(dukat::PerformanceCounter::MESHES)
				<< " VERT: " << perfc.avg(dukat::PerformanceCounter::VERTICES)
				<< " BB: " << perfc.avg(PerformanceCounter::BB_CHECKS)
				<< std::endl;
			debug_text->set_text(ss.str());
		}, true);

		game->set_controller(this);
	}

	void CollisionScene::handle_keyboard(const SDL_Event & e)
	{
		switch (e.key.keysym.sym)
		{
		case SDLK_ESCAPE:
			game->set_done(true);
			break;
		case SDLK_MINUS:
			remove_object();
			break;
		case SDLK_EQUALS:
			add_object();
			break;
		case SDLK_SPACE:
			animate = !animate;
			break;

		case SDLK_g:
			show_grid = !show_grid;
			if (show_grid)
			{
				debug_effect->set_flags(static_cast<DebugEffect2::Flags>(DebugEffect2::Flags::BODIES | DebugEffect2::Flags::GRID));
			}
			else
			{
				debug_effect->set_flags(DebugEffect2::Flags::BODIES);
			}
			break;
		}
	}

	void CollisionScene::remove_object(void)
	{
		if (!objects.empty())
		{
			game->get<CollisionManager2>()->destroy_body(objects.back()->body);
			objects.pop_back();
		}
	}

	void CollisionScene::add_object(void)
	{
		auto dir = Vector2{ randf(-max_speed, max_speed), randf(-max_speed, max_speed) };
		auto pos = Vector2::random(-screen_dim, screen_dim);
		auto size = randf(10.0f, 20.0f);
		auto body = game->get<CollisionManager2>()->create_body();
		body->bb.min = pos - Vector2{ size, size };
		body->bb.max = pos + Vector2{ size, size };
		objects.push_back(std::make_unique<GameObject>(dir, body));
	}

	void CollisionScene::update(float delta)
	{
		auto dev = game->get_devices()->active;
		cursor->p.x = dev->rxa - screen_dim.x;
		cursor->p.y = dev->rya - screen_dim.y;

		if (animate)
		{
			for (auto& o : objects)
			{
				o->body->bb.min += o->dir * delta;
				o->body->bb.max += o->dir * delta;
			}
		}

		Scene2::update(delta);
	}

	GameObject::GameObject(const Vector2& dir, CollisionManager2::Body* body) : dir(dir), body(body)
	{
		body->owner = this;
		this->subscribe(Events::CollisionBegin, this);
	}

	GameObject::~GameObject(void)
	{
		this->unsubscribe(Events::CollisionBegin, this);
	}

	void GameObject::receive(const Message& msg)
	{
		switch (msg.event)
		{
		case Events::CollisionBegin:
			auto other_body = static_cast<const CollisionManager2::Body*>(msg.param1);
			auto collision = static_cast<const Collision*>(msg.param2);
			if (body->dynamic && body->solid && other_body->solid)
			{
				if (collision->normal.x != 0.0f)
				{
					dir.x = -dir.x;
				}
				else
				{
					dir.y = -dir.y;
				}
			}
			break;
		}
	}
}

int main(int argc, char** argv)
{
	try
	{
		std::string config = "../assets/collision.ini";
		if (argc > 1)
		{
			config = argv[1];
		}
		dukat::Settings settings(config);
		dukat::Game2 app(settings);
		app.add_scene("main", std::make_unique<dukat::CollisionScene>(&app));
		app.push_scene("main");
		return app.run();
	}
	catch (const std::exception& e)
	{
		dukat::logger << "Application failed with error." << std::endl << e.what() << std::endl;
		return -1;
	}
	return 0;
}