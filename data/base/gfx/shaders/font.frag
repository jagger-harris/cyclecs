#version 330

in vec2 uv;
in vec4 tint;
out vec4 frag_color;

uniform sampler2D u_tex;

void main() {
    float distance = texture(u_tex, uv).r;
    float sdf = distance;
    float width = fwidth(sdf);
    float alpha = smoothstep(0.5f - width, 0.5f + width, sdf);
    if (alpha < 0.01f)
        discard;

    frag_color = vec4(tint.rgb, tint.a * alpha);
}
