#version 330 core

in vec2 uv_out;
out vec4 frag_color;
uniform sampler2D texture1;

void main() {
    frag_color = texture(texture1, uv_out);
}
