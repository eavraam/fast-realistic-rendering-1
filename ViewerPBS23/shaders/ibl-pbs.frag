#version 330

// Inputs
in vec3 frag_pos;           // Vertex position
in vec3 m_normal;           // Normals

// Uniforms
// - General
uniform vec3 light;         // import light position
uniform vec3 camPos;        // camera position // In our case it is always 0, but I still import it through uniform for readability
// - Material
uniform vec3 fresnel;       // Import the Fresnel F0
uniform vec3 albedo;        // Import material albedo
uniform float roughness;    // Import material roughness
uniform float metalness;    // Import material metalness
// - IBL
uniform samplerCube specular_map;   // Import the specular cubemap texture
uniform samplerCube diffuse_map;    // Import the diffuse cubemap texture
uniform sampler2D brdfLUT_map;      // Import the brdfLUT texture

// Outputs
out vec4 frag_color;

// Fragment shader variables
const float PI = 3.14159265359;                  // Math Pi

// GGX / Trowbridge-Reitz Normal Distribution Function
float D(float a, vec3 N, vec3 H)
{
    float numerator = pow(a, 2.0);             // Numerator = a^2

    float NdotH = max(dot(N,H), 0.0);          // We only need the >=0 3D field, not "underground"

    float denominator = PI * pow(pow(NdotH, 2.0) * (pow(a, 2.0) - 1.0) + 1.0, 2.0);
    denominator = max(denominator, 0.000001);   // Prevent division with 0

    return numerator / denominator;
}

// Schlick-Beckmann Geometry Shadowing Function
float G1(float a, vec3 N, vec3 X)
{
    float numerator = max(dot(N,X), 0.0);                      //We only need the >=0 3D field, not "underground"

    float k = a / 2.0;
    float denominator = max(dot(N,X), 0.0) * (1.0 - k) + k;
    denominator = max(denominator, 0.000001);                   // Prevent division with 0

    return numerator / denominator;
}

// Smith Model
float G(float a, vec3 N, vec3 V, vec3 L)
{
    return G1(a, N, V) * G1(a, N, L);
}

// Fresnel-Schlick Function
vec3 F(vec3 F0, vec3 N, vec3 H)
{
    return F0 + (vec3(1.0) - F0) * pow(1.0 - max(dot(N,H), 0.0), 5.0);
}

vec3 F_Roughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Rendering equation for ONE lightsource
vec3 PBR()
{
    // Main Vectors
    vec3 N = normalize(m_normal);               // Normal vector
    vec3 V = normalize(frag_pos - camPos);      // View vector
    vec3 R = reflect(-V, N);                    // Reflection vector

    // Initializations - General
    vec3 F0 = fresnel;                    // Import the F0 from the fresnel uniform
    F0 = mix(F0, albedo, metalness);      // Mix the fresnel F0

    // Diffuse - Specular K-Terms
    vec3 Ks = F_Roughness(max(dot(N, V), 0.0), F0, roughness);
    vec3 Kd = (vec3(1.0) - Ks) * (1.0 - metalness);

    // Ambient / Diffuse Part
    vec3 irradiance = texture(diffuse_map, N).rgb;
    vec3 diffuse = irradiance * albedo;
    vec3 ambient = Kd * diffuse;

    // Specular Part
    const float MAX_REFLECTION_LOD = 7.0;
    vec3 prefilteredColor = textureLod(specular_map, R,  roughness * MAX_REFLECTION_LOD).rgb;
    vec2 environmentBRDF  = texture(brdfLUT_map, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (Ks * environmentBRDF.x + environmentBRDF.y);

    // Get the light output BEFORE upgrading it with HDR and Gamma correction
    vec3 temp = ambient + specular;
    // --- Limitations correction --- //
    // HDR Tone mapping, compress the range of brightness values to fit the display
    vec3 totalLight = temp / (temp + vec3(1.0));
    // Gamma correction, non-linear gamma-encoded color space
    totalLight = pow(totalLight, vec3(1.0/2.2));

    return temp;
}


void main()
{
    vec3 result = PBR();
    frag_color = vec4(result, 1.0);
}

