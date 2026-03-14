#version 330

in vec2 uv;
in vec4 tint;
out vec4 frag_color;

uniform sampler2D u_tex;

void main() {
    vec4 tex = texture(u_tex, uv);
    frag_color = vec4(tex) * tint;
}
