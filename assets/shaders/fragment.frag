#version 330 core

in vec3 v_FragPos;
in vec3 v_Normal;
in vec4 v_LightClip;

uniform vec3 u_CameraPos;

uniform vec3  u_LightPos;
uniform vec3  u_LightColor;
uniform float u_LightIntensity;

uniform vec3  u_BaseColor;
uniform float u_Roughness;
uniform float u_Metallic;
uniform vec3  u_EmissiveColor;
uniform float u_EmissiveStrength;

// --- Shadow ---
uniform sampler2DShadow u_ShadowMap;
uniform float           u_ShadowBias;
uniform float           u_ShadowTexelSize;

out vec4 FragColor;

float saturate(float x) { return clamp(x, 0.0, 1.0); }
vec3  saturate(vec3  x) { return clamp(x, 0.0, 1.0); }

float D_GGX(float NdotH, float a)
{
    float a2 = a * a;
    float d  = (NdotH*NdotH) * (a2 - 1.0) + 1.0;
    return a2 / (3.14159265 * d * d + 1e-7);
}

float G_SmithGGXCorrelated(float NdotV, float NdotL, float a)
{
    float a2  = a * a;
    float gv  = NdotV + sqrt(NdotV * (NdotV - NdotV*a2) + a2);
    float gl  = NdotL + sqrt(NdotL * (NdotL - NdotL*a2) + a2);
    return 1.0 / (gv * gl + 1e-7);
}

vec3 F_Schlick(vec3 F0, float VdotH)
{
    return F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);
}

// --- Shadow sampling (PCF 3x3) ---
float computeShadow(float NdotL)
{
    // Clip -> NDC -> [0,1]
    vec3 proj = v_LightClip.xyz / v_LightClip.w;
    proj = proj * 0.5 + 0.5;

    // Hors de la shadow map => éclairé
    if (proj.z > 1.0 || proj.x < 0.0 || proj.x > 1.0 || proj.y < 0.0 || proj.y > 1.0)
        return 1.0;

    // Petit bias dépendant du slope pour réduire l'acné
    float bias = max(u_ShadowBias * (1.0 - NdotL), 0.0005);

    float sum = 0.0;
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            vec2 off = vec2(float(x), float(y)) * u_ShadowTexelSize;
            // sampler2DShadow fait la comparaison depthRef <= texDepth
            sum += texture(u_ShadowMap, vec3(proj.xy + off, proj.z - bias));
        }
    }
    return sum / 9.0; // visibilité (1 = éclairé, 0 = dans l'ombre)
}

void main()
{
    // Basis
    vec3 N = normalize(v_Normal);
    vec3 V = normalize(u_CameraPos - v_FragPos);

    // light direction
    vec3 L = normalize(u_LightPos - v_FragPos);
    vec3 H = normalize(V + L);

    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));
    float NdotH = saturate(dot(N, H));
    float VdotH = saturate(dot(V, H));

    if (NdotL <= 0.0 || NdotV <= 0.0) {
        vec3 emissive = u_EmissiveColor * u_EmissiveStrength;
        FragColor = vec4(emissive, 1.0);
        return;
    }

    // material properties
    float rough = clamp(u_Roughness, 0.04, 1.0);   // clamp to avoid artifacts
    float metal = saturate(u_Metallic);
    float a     = rough * rough;

    vec3  albedo = saturate(u_BaseColor);
    vec3  F0     = mix(vec3(0.04), albedo, metal);

    float  D = D_GGX(NdotH, a);
    float  G = G_SmithGGXCorrelated(NdotV, NdotL, a);
    vec3   F = F_Schlick(F0, VdotH);

    vec3  specular = (D * G) * F / (4.0 * NdotV * NdotL + 1e-7);
    vec3  kd = (1.0 - F) * (1.0 - metal);
    vec3  diffuse = kd * albedo / 3.14159265;

    vec3 light = u_LightColor * u_LightIntensity;

    // --- Shadow visibility ---
    float visibility = computeShadow(NdotL);

    // Combine diffuse and specular contributions (shadow appliquée ici)
    vec3 color = (diffuse + specular) * light * NdotL * visibility;

    // Add emissive contribution
    color += u_EmissiveColor * u_EmissiveStrength;

    color = max(color, vec3(0.0));
    FragColor = vec4(color, 1.0);
}
