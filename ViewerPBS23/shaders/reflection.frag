#version 330 core

// Inputs
in vec3 m_normal;
in vec3 frag_pos;

// Uniforms
uniform vec3 camPos;
uniform samplerCube specular_map;

// Output
out vec4 frag_color;

void main()
{
    vec3 I = normalize(camPos - frag_pos);
    vec3 R = reflect(I, normalize(m_normal));
    frag_color = vec4(texture(specular_map, R).rgb, 1.0);
}
