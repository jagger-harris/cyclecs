#version 330

layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;
layout(location = 3) in mat4 a_instance_mvp;
layout(location = 7) in vec4 a_instance_tint;

out vec3 normal;
out vec2 uv;
out vec4 tint;

uniform mat4 u_mvp;
uniform vec4 u_tint;
uniform bool u_use_instancing;

void main() {
    mat4 current_mvp = u_use_instancing ? a_instance_mvp : u_mvp;
    gl_Position = current_mvp * vec4(a_pos, 1.0f);
    uv = a_uv;
    tint = u_use_instancing ? a_instance_tint : u_tint;
}
