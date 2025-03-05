#version 330 core

layout (location = 0) in vec3 position;

out vec2 frag_position;

uniform mat4 mvp;

void main() {
  gl_Position = mvp * vec4(position, 1.0f);
  frag_position = position.xy * 0.5 + 0.5;
}
