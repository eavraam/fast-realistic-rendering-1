#version 330

// Layout
layout (location = 0) in vec3 vert;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;

// Uniforms
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normal_matrix;

// Outputs
out vec3 frag_pos;
out vec3 m_normal;

void main(void)  {

    // Pass the normals to the fragment shader
    m_normal = normal;
    // Pass the position to the fragment shader
    frag_pos = vec3(model * vec4(vert, 1.0f));
    gl_Position = projection * view * vec4(frag_pos, 1.0f);
}
