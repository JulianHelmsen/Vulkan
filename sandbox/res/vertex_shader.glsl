#version 450

layout(location = 0) in vec3 position;

layout(push_constant) uniform camera_object{
    mat4 view_projection_matrix;
    mat4 model_matrix;

}camera;

layout(location = 0) out vec3 f_world_pos;


void main() {
    vec4 world_pos = camera.model_matrix * vec4(position, 1.0f);
    f_world_pos = world_pos.xyz;
    gl_Position = camera.view_projection_matrix * world_pos;
}