#version 330

// Inputs
in vec3 frag_pos;       // import position
in vec3 m_normal;       // import normal
in vec3 face_normal;    // import face_normals
in vec2 v_uv;          // import texture coordinates

// Uniforms
uniform vec3 light;     // import light position
uniform vec3 camPos;    // camera position // In our case it is always 0, but I still import it through uniform

uniform sampler2D color_map;
uniform sampler2D roughness_map;
uniform sampler2D metalness_map;

// Outputs
out vec4 frag_color;

// Per-Fragment Phong Lighting
void main (void) {
    // Variables
    vec3 lightColor = vec3 (1.0f);
    vec3 objectColor = vec3(0.1f, 0.16f, 0.62f);

    // ambient
    float ambientStrength = 0.4f;
    //vec3 ambient = ambientStrength * lightColor;    // Compute the diffuse parameter of the lighting
    vec3 ambient = ambientStrength * lightColor * vec3(texture(color_map, v_uv));      // Add the texture's albedo texture for the ambient computation

    // diffuse
    vec3 lightDir = normalize(light - frag_pos);
    float diff = max(dot(m_normal, lightDir), 0.0f);
    //vec3 diffuse = diff * lightColor; // Compute the diffuse parameter of the lighting
    vec3 diffuse = diff * lightColor * vec3(texture(color_map, v_uv));  // Add the texture's albedo texture for the ambient computation

    // specular
    float specularStrength = 1.0f;
    vec3 viewDir = normalize(camPos - frag_pos);
    vec3 reflectDir = reflect(lightDir, m_normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0f), 64.0f);
    //vec3 specular = specularStrength * spec * lightColor;   // Compute the specular parameter of the lighting
    vec3 specular = specularStrength * spec * lightColor * vec3(texture(metalness_map, v_uv));    // Add the texture's albedo texture for the ambient computation

    //vec3 result = (ambient + diffuse + specular) * objectColor;   // First version, for simple Phong lighting on a BLUE sphere
    vec3 result = ambient + diffuse + specular;     // This version uses the material's diffuse and metallic
    frag_color = vec4(result, 1.0f);
}

// Per-Face Phong Lighting
//--------------------------------
//The main difference in the code is that we compute the face normal
//using the cross function and the derivatives of the fragment position
//using dFdx and dFdy instead of using the interpolated normal.
//We then use the face normal to compute the lighting values instead of the interpolated normal.

// We assume that the vertex normals are already provided in world space
//and are used to compute the face normal.
//--------------------------------

//void main (void) {
//    // Compute the face normal
//    vec3 norm = normalize(cross(dFdx(frag_pos), dFdy(frag_pos)));

//    // ambient
//    float ambientStrength = 0.4f;
//    vec3 ambient = ambientStrength * lightColor;

//    // diffuse
//    vec3 lightDir = normalize(light - frag_pos);
//    float diff = max(dot(norm, lightDir), 0.0f);
//    vec3 diffuse = diff * lightColor;

//    // specular
//    float specularStrength = 5.5f;
//    vec3 viewDir = normalize(camPos - frag_pos);
//    vec3 reflectDir = reflect(lightDir, norm);
//    float spec = pow(max(dot(viewDir, reflectDir), 0.0f), 64.0f);
//    vec3 specular = specularStrength * spec * lightColor;

//    vec3 result = (ambient + diffuse + specular) * objectColor;
//    frag_color = vec4(result, 1.0f);
//}

