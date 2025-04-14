#version 430 core

layout (location = 0) in vec2 a_pos; 
out vec2 tex_cord;

layout(location = 1) uniform float u_zoom;
layout(location = 2) uniform vec2 u_padd;

void main() { 
    vec2 pos = (a_pos * u_zoom) + u_padd;
    gl_Position = vec4(pos, 0, 1); 
    tex_cord = (a_pos * 0.5) + 0.5;
}

