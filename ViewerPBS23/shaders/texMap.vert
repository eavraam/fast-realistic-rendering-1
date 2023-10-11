#version 330

// Layout
layout (location = 0) in vec3 vert;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;

// Uniforms
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Outputs
out vec2 v_uv;
out vec3 v_normals;

void main(void)  {
    // Not exactly needed right now for texturing the sphere, might need later
    // Transform vertex position and normal to world space
    vec4 worldNormal = model * vec4(normal, 0.0);
    // Pass normal and texture coordinate to fragment shader
    v_normals = normalize(worldNormal.xyz);


    // Transform vertex position to clip space
    gl_Position = projection * view * model * vec4(vert, 1.0f);
    // Pass texture coordinates to the fragment shader
    v_uv = texCoord;
}
