#version 430 core

out vec4 color;
in vec2 tex_cord;

layout(location = 0) uniform sampler2D u_texture;

void main() {
    color = texture(u_texture, tex_cord);
}
