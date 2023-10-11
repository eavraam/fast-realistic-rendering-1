#version 330

// Inputs
in vec2 v_uv;
in vec3 v_normals;

// Uniforms
uniform int current_texture;
uniform sampler2D color_map;
uniform sampler2D roughness_map;
uniform sampler2D metalness_map;

// Outputs
out vec4 frag_color;

void main(void)
{
    if(current_texture==0)
    {
        frag_color = texture(color_map, v_uv);
    }
    else if(current_texture==1)
    {
        frag_color = texture(roughness_map, v_uv);
    }
    else if(current_texture==2)
    {
        frag_color = texture(metalness_map, v_uv);
    }

}
