#version 330

in vec2 uv;
in vec4 tint;
out vec4 frag_color;

uniform sampler2D u_tex;

void main() {
    float distance = texture(u_tex, uv).r;
    float sdf = distance;
    float edge = 0.5;
    float width = fwidth(sdf);
    float alpha = smoothstep(edge - width, edge + width, sdf);

    frag_color = vec4(tint.rgb, tint.a * alpha);
}
