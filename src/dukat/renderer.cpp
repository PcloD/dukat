#include "stdafx.h"
#include "color.h"
#include "log.h"
#include "perfcounter.h"
#include "renderer.h"
#include "shadercache.h"
#include "sysutil.h"
#include "buffers.h"

namespace dukat
{
	Renderer::Renderer(Window* window, ShaderCache* shader_cache)
		: window(window), shader_cache(shader_cache), active_program(0), show_wireframe(false), blending(false)
	{
		window->subscribe(Events::WindowResized, this);
		test_capabilities();
		uniform_buffers = std::make_unique<GenericBuffer>(UniformBuffer::_COUNT);
		// Default settings
		set_clear_color(Color{ 0.0f, 0.0f, 0.0f, 0.0f });
		// Enable back-face culling
		glFrontFace(GL_CCW);
		set_backface_culling(true);
		gl_check_error();
	}

	Renderer::~Renderer(void)
	{
		window->unsubscribe(Events::WindowResized, this);
	}

	void Renderer::test_capabilities(void)
	{
		GLint int_val;
		log->debug("Testing rendering capabilities.");;

		auto shader_version = std::string((char*)glGetString(GL_SHADING_LANGUAGE_VERSION));
		log->debug("GL_SHADING_LANGUAGE_VERSION: {}", shader_version);
		
		// Check capabilities
		glGetIntegerv(GL_MAX_TEXTURE_UNITS, &int_val);
		log->debug("GL_MAX_TEXTURE_UNITS: {}", int_val);
		assert(int_val >= Renderer::max_texture_units);
		glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &int_val);
		log->debug("GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS: {}", int_val);
		glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &int_val);
		log->debug("GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS: {}", int_val);
		
#if OPENGL_VERSION >= 30
		glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &int_val);
		log->debug("GL_MAX_UNIFORM_BUFFER_BINDINGS: {}", int_val);
		assert(int_val >= UniformBuffer::_COUNT);
		glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &int_val);
		log->debug("GL_MAX_UNIFORM_BLOCK_SIZE: {}", int_val);
#endif

#if OPENGL_VERSION >= 42
		// TODO: validate with ctx 4.x
		glGetInternalformativ(GL_TEXTURE_2D, GL_RGBA8, GL_INTERNALFORMAT_PREFERRED, 1, &int_val);
		log->debug("Preferred format for GL_RGBA8: {}", int_val);
		glGetInternalformativ(GL_TEXTURE_2D, GL_RGB8, GL_INTERNALFORMAT_PREFERRED, 1, &int_val);
		log->debug("Preferred format for GL_RGB8: {}", int_val);
		glGetInternalformativ(GL_TEXTURE_2D, GL_RGBA16, GL_INTERNALFORMAT_PREFERRED, 1, &int_val);
		log->debug("Preferred format for GL_RGBA16: {}", int_val);
		glGetInternalformativ(GL_TEXTURE_2D, GL_RGB16, GL_INTERNALFORMAT_PREFERRED, 1, &int_val);
		log->debug("Preferred format for GL_RGB16: {}", int_val);
#endif

#if OPENGL_VERSION >= 43
		glGetIntegerv(GL_MAX_UNIFORM_LOCATIONS, &int_val);
		log->debug("GL_MAX_UNIFORM_LOCATIONS: {}", int_val);
#endif

		// Check supported extensions
		log->debug("GL_EXT_geometry_shader4: {}",
			(glewIsExtensionSupported("GL_EXT_geometry_shader4") ? "yes" : "no"));
		log->debug("GL_EXT_texture_filter_anisotropic: {}",
			(glewIsExtensionSupported("GL_EXT_texture_filter_anisotropic") ? "yes" : "no"));
		log->debug("GL_EXT_vertex_array_object: {}",
			(glewIsExtensionSupported("GL_EXT_vertex_array_object") ? "yes" : "no"));
		log->debug("GL_EXT_framebuffer_object: {}",
			(glewIsExtensionSupported("GL_EXT_framebuffer_object") ? "yes" : "no"));

#ifdef _DEBUG
		gl_check_error();
#endif
	}

	void Renderer::receive(const Message& msg)
	{
		switch (msg.event)
		{
		case Events::WindowResized:
			glViewport(0, 0, *static_cast<const int*>(msg.param1), *static_cast<const int*>(msg.param2));
			break;
		}
	}

	void Renderer::switch_shader(ShaderProgram* program)
	{
		assert(program != nullptr);
		if (active_program != program)
		{
			glUseProgram(program->id);
			perfc.inc(PerformanceCounter::SHADERS);
			this->active_program = program;
#if OPENGL_VERSION >= 30
			// re-bind uniform blocks
			auto cidx = glGetUniformBlockIndex(program->id, Renderer::uf_camera);
			if (cidx != GL_INVALID_INDEX)
			{
				glUniformBlockBinding(program->id, cidx, UniformBuffer::CAMERA);
				glBindBufferBase(GL_UNIFORM_BUFFER, UniformBuffer::CAMERA, uniform_buffers->buffers[UniformBuffer::CAMERA]);
			}
			auto lidx = glGetUniformBlockIndex(program->id, Renderer::uf_light);
			if (lidx != GL_INVALID_INDEX)
			{
				glUniformBlockBinding(program->id, lidx, UniformBuffer::LIGHT);
				glBindBufferBase(GL_UNIFORM_BUFFER, UniformBuffer::LIGHT, uniform_buffers->buffers[UniformBuffer::LIGHT]);
			}
#else
			// update all uniforms
			update_uniforms();
#endif
		}
	}

	void Renderer::bind_uniform(UniformBuffer buffer, GLsizeiptr size, const GLvoid* data)
	{
		glBindBufferBase(GL_UNIFORM_BUFFER, buffer, uniform_buffers->buffers[buffer]);
		glBufferData(GL_UNIFORM_BUFFER, size, data, GL_STREAM_DRAW);
	}

	void Renderer::set_wireframe(bool wireframe)
	{
		show_wireframe = wireframe;
	}

	void Renderer::set_backface_culling(bool backface_culling)
	{
		this->backface_culling = backface_culling;
		if (backface_culling)
		{
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
		}
		else
		{
			glDisable(GL_CULL_FACE);
			glPolygonMode(GL_BACK, GL_LINE);
			glPolygonMode(GL_FRONT, GL_FILL);
		}
	}

	void Renderer::set_blending(bool blending)
	{
		this->blending = blending;
		if (blending)
		{
			glEnable(GL_BLEND);
		}
		else
		{
			glDisable(GL_BLEND);
		}
	}

	void Renderer::set_clear_color(const Color & clr)
	{
		glClearColor(clr.r, clr.g, clr.b, clr.a);
	}
}