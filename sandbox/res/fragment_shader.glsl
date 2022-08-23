#version 450

layout(location = 0) out vec4 out_color;


layout(location = 0) in vec3 f_world_pos;

void main() {
	out_color = vec4(f_world_pos, 1.0f);
}