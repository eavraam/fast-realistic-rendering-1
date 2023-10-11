#version 330

// Layouts
layout (location = 0) in vec3 vert;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;

// Uniforms
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Outputs
out vec3 frag_pos;
out vec3 m_normal;

void main(void)  {
    m_normal = mat3(transpose(inverse(model))) * normal;
    frag_pos = vec3(model * vec4(vert, 1.0f));
    gl_Position = projection * view * vec4(frag_pos, 1.0f);
}
