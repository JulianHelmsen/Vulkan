#version 450

layout(location = 0) in vec3 position;

vec2 positions[3] = vec2[](
	vec2(0.0f, 0.0f), vec2(1.0f, 1.0f), vec2(1.0f, 0.0f)
);

void main() {
	gl_Position = vec4(positions[gl_VertexIndex], 0.0f, 1.0f);
}