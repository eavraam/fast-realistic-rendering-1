#version 330

layout (location = 0) in vec3 vert;

out vec3 vert_pos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main(void)  {
    gl_Position = projection * view * vec4(vert, 1.0);
    vert_pos = vert;
}
