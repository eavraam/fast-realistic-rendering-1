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
out vec3 face_normal;
out vec2 v_uv;

void main(void)  {
    // If I always want to see the front of the sphere, while rotating the background
    //m_normal = normalize(normal_matrix * normal);
    // If I want to rotate everything, along with the sphere
    m_normal = normal;

    // By first multiplying the vertex position vert by the model matrix model and converting it to a homogeneous coordinate
    // by adding a 1 in the fourth component, we obtain the position of the vertex in world space, which we can then transform
    // to view space by multiplying it with the view matrix.
    frag_pos = vec3(model * vec4(vert, 1.0f));
    gl_Position = projection * view * vec4(frag_pos, 1.0f);

    // Calculate the normal for the face
    face_normal = normalize(normal_matrix * normal);

    // Pass the texture coordinates to the fragment shader
    v_uv = texCoord;
}
