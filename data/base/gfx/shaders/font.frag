#version 330

in vec2 uv;
in vec4 tint;
out vec4 frag_color;

uniform sampler2D u_tex;

void main() {
    float alpha = texture(u_tex, uv).r;
    if (alpha < 0.01f)
        discard;

    frag_color = vec4(tint.rgb, tint.a * alpha);
}
