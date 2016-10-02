#pragma once

#include <memory>
#include "gamebase.h"
#include "renderer2.h"

namespace dukat
{
	// Game implementation using the 2D subsystem.
	class Game2 : public GameBase
	{
	protected:
		std::unique_ptr<Renderer2> renderer;
		virtual void init(void);
		virtual void update(float delta);
		virtual void render(void);
		void toggle_debug(void);

	public:
		Game2(const Settings& settings);
		virtual ~Game2(void) { }

		std::unique_ptr<TextMeshInstance> create_text_mesh(float size = 1.0f, const Color& color = { 1.0f, 1.0f, 1.0f, 1.0f });
		Renderer2* get_renderer(void) const { return renderer.get(); }
	};
}