#version 330

in vec3 vert_pos;
out vec4 frag_color;

uniform samplerCube specular_map;

void main (void) {
    frag_color = texture(specular_map, vert_pos);
    // frag_color = vec4(0.3, 0.3, 0.3, 1.0);
}
