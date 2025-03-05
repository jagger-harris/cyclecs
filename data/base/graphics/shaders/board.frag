#version 330 core

in vec2 frag_position;
out vec4 frag_color;

uniform vec4 object_color;
uniform vec4 border_color;
uniform float border_thickness;
uniform float corner_radius;
uniform float aa;

void main() {
  // Center the coordinates around (0.5, 0.5) to simplify calculations
  vec2 centered_pos = frag_position - vec2(0.5);

  // Define the inner side length (excluding the border thickness)
  float half_side = 0.5 - corner_radius;

  // Calculate signed distance to the square with rounded corners
  vec2 abs_pos = abs(centered_pos);
  vec2 d = abs_pos - vec2(half_side);

  // Distance to the outer edge of the border with rounded corners
  float dist = length(max(d, 0.0)) + min(max(d.x, d.y), 0.0) - corner_radius;

  // Calculate smooth anti-aliasing on the edge based on distance to the border
  float alpha;
  if (dist < -(border_thickness)) {
    // Inside the square area (no border)
    frag_color = object_color;
  } else if (dist < 0.0) {
    // Within the border area, apply smooth transition
    alpha = smoothstep(-aa, 0.0, dist + border_thickness);
    frag_color = mix(object_color, border_color, alpha);
  } else {
    // Outside the square (background)
    alpha = smoothstep(0.0, aa, dist);
    frag_color =
        mix(border_color, vec4(0.0), alpha); // Background as transparent
  }
}
