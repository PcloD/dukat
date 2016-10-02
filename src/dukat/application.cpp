#include "stdafx.h"
#include "application.h"
#include "log.h"
#include "perfcounter.h"
#include "sysutil.h"
#include "window.h"
#include "devicemanager.h"
#include "keyboarddevice.h"
#include "settings.h"

namespace dukat
{
	Application::Application(const Settings& settings)
		: settings(settings), title(settings.get_string("window.title")), paused(false), done(false)
	{
		sdl_check_result(SDL_Init(SDL_INIT_EVERYTHING), "Initialize SDL");
		window = std::make_unique<Window>(settings.get_int("window.width", 640), 
			settings.get_int("window.height", 480), settings.get_bool("window.fullscreen"));
		window->set_title(title);
		device_manager = std::make_unique<DeviceManager>();
		device_manager->add_keyboard(window.get());
		device_manager->joystick_support = settings.get_bool("input.joystick.support", true);
	}

	Application::~Application(void)
	{
		SDL_Quit();
	}

	int Application::run(void)
	{
		logger << "Initializing application." << std::endl;
		init();

		logger << "Entering application loop." << std::endl;		
		Uint32 ticks, last_update = 0, last_frame = 0;
		SDL_Event e;
		while (!done)
		{
			ticks = SDL_GetTicks();

			if (!paused)
			{
				device_manager->update();
				update(((float)(ticks - last_update)) / 1000.0f);
				last_update = SDL_GetTicks();
			}

			// update FPS counter
			if (ticks - last_frame >= 1000)
			{
				last_fps = perfc.sum(PerformanceCounter::FRAMES);
				last_frame = ticks;
				perfc.collect_stats();
			}

			// process events
			while (SDL_PollEvent(&e))
			{
				handle_event(e);
			}
			
			// render to screen
			render();
			perfc.inc(PerformanceCounter::FRAMES);
			perfc.reset();
		}

		release();

		return 0;
	}

	void Application::handle_event(const SDL_Event& e)
	{
		switch (e.type)
		{
		case SDL_QUIT:
			done = true;
			break;
		case SDL_KEYDOWN:
			handle_keyboard(e);
			break;
		case SDL_JOYDEVICEADDED:
			device_manager->add_joystick(e.jdevice.which);
			break;
		case SDL_JOYDEVICEREMOVED:
			device_manager->remove_joystick(e.jdevice.which);
			break;
		}
	}

	void Application::handle_keyboard(const SDL_Event& e)
	{
		switch (e.key.keysym.sym)
		{
		case SDLK_ESCAPE:
			done = true;
			break;
		case SDLK_RETURN:
			if (e.key.keysym.mod & KMOD_ALT)
			{
				window->toggle_fullscreen();
			}
			break;
		}
	}

}