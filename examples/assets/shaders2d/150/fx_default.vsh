#version 150
///
// Fullscreen fx pass vertex shader.
///
in vec4 a_position;
in vec2 a_tex_coord;

out vec2 v_tex_coord;

void main()
{
    gl_Position = a_position;
    v_tex_coord = a_tex_coord;
}