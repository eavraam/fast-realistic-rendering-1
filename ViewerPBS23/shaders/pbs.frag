#version 330

// Inputs
in vec3 frag_pos;           // Vertex position
in vec3 m_normal;           // Normals
in vec2 v_uv;               // Texture Coordinates

// Uniforms
uniform vec3 light;         // import light position
uniform vec3 camPos;        // camera position // In our case it is always 0, but I still import it through uniform
uniform vec3 fresnel;       // Import the Fresnel F0
// --- material values --- //
uniform vec3 albedo;        // Import material albedo
uniform float roughness;   // Import material roughness
uniform float metalness;   // Import material metalness
// --- material maps --- //
//uniform sampler2D color_map;
//uniform sampler2D roughness_map;
//uniform sampler2D metalness_map;

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
vec3 F(vec3 F0, vec3 L, vec3 H)
{
    return F0 + (vec3(1.0) - F0) * pow(1.0 - max(dot(L,H), 0.0), 5.0);
}

// Rendering equation for ONE lightsource
vec3 PBR()
{
    // Main Vectors
    vec3 N = normalize(m_normal);               // Normal vector
    vec3 V = normalize(frag_pos - camPos);      // View vector
    vec3 L = normalize(light - frag_pos);       // For POINT and SPOT lights
    //vec3 L = normalize(light);                  // For DIRECTIONAL lights (normalizing position)
    vec3 H = normalize(V + L);                  // Half-way vector
    vec3 R = reflect(V, N);                     // Reflection

    // Initializations - General
    // -- Using material maps. For material values, delete albedo, roughness, metalness here
    // -- and uncomment the respective values on the top of the code (uniforms).

    // The albedo textures are generally authored in sRGB space, so we usually need to convert them
    // into linear space before using albedo in the lighting calculations. If not, comment the first
    // albedo declaration and uncomment the next.
    //vec3 albedo = pow(texture(color_map, v_uv).rgb, vec3(2.2));
    //vec3 albedo = texture(color_map, v_uv).rgb;
    //float roughness = texture(roughness_map, v_uv).r;
    //float metalness = texture(metalness_map, v_uv).r;

    float lightIntensity = 1.0f;
    float a = pow(roughness, 2.0);        // Initialize a=roughness^2
    vec3 F0 = fresnel;                    // Import the F0 from the fresnel uniform
    F0 = mix(F0, albedo, metalness);      // Mix the fresnel F0

    // Diffuse - Specular Terms
    vec3 Ks = F(F0, L, H);
    vec3 Kd = (vec3(1.0) - Ks) * (1.0 - metalness);

    // Lambert
    vec3 Fd = pow(albedo, vec3(2.2)) / PI;

    // Cook-Torrance
    vec3 Fs_numerator = D(a, N, H) * G(a, N, V, L) * F(F0, L, H);

    float Fs_denominator = 4.0 * max(dot(V,N), 0.0) * max(dot(L,N), 0.0);    //We only need the >=0 3D field, not "underground"
    Fs_denominator = max(Fs_denominator, 0.000001);                          // Prevent division by 0

    // Final result of the Cook-Torrance method
    vec3 Fs_CookTorrance = Fs_numerator / Fs_denominator;

    vec3 BRDF = Kd*Fd + Fs_CookTorrance;
    vec3 outgoingLight = BRDF * lightIntensity * max(dot(L,N), 0.0f);       // Without emissivity

    // --- Limitations correction --- //
    // HDR Tone mapping, compress the range of brightness values to fit the display
    vec3 totalLight = outgoingLight / (outgoingLight + vec3(1.0));
    // Gamma correction, non-linear gamma-encoded color space
    totalLight = pow(totalLight, vec3(1.0/2.2));

    return totalLight;
}


void main()
{
    vec3 result = PBR();
    frag_color = vec4(result, 1.0);
}

