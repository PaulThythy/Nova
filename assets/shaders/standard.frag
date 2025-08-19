#version 330 core

in vec3 v_FragPos;
in vec3 v_Normal;
in vec4 v_LightClip;

out vec4 FragColor;

// Camera / material
uniform vec3  u_CameraPos;

uniform vec3  u_BaseColor;
uniform float u_Roughness;
uniform float u_Metallic;
uniform vec3  u_EmissiveColor;
uniform float u_EmissiveStrength;

// Light types
#define LIGHT_DIRECTIONAL 0
#define LIGHT_SPOT        1
#define LIGHT_POINT       2

// Light (common)
uniform int   u_LightType;
uniform vec3  u_LightColor;
uniform float u_LightIntensity;
uniform bool  u_LightShadows;

// Light (per-type)
uniform vec3  u_LightPos;      // Spot/Point (world)
uniform vec3  u_LightDir;      // Directional/Spot (world, normalized)
uniform float u_LightRange;    // Spot/Point
uniform float u_SpotInnerCos;  // Spot (cos(InnerAngle))
uniform float u_SpotOuterCos;  // Spot (cos(OuterAngle))

// Shadow map (directional only)
uniform sampler2DShadow u_ShadowMap;
uniform vec2            u_ShadowTexelSize; // 1/textureSize

// ----------------- Helpers -----------------
float attenuationDistance(float d, float range) {
    float x = clamp(1.0 - (d*d)/(range*range), 0.0, 1.0);
    return x * x;
}
float spotFactor(vec3 L, vec3 spotDir, float innerCos, float outerCos) {
    float cd = dot(normalize(-L), normalize(spotDir));
    return smoothstep(outerCos, innerCos, cd);
}
float computeShadow(float NdotL) {
    if (!u_LightShadows) return 1.0;
    vec3 proj = v_LightClip.xyz / v_LightClip.w;
    proj = proj * 0.5 + 0.5;
    if (proj.x < 0.0 || proj.x > 1.0 || proj.y < 0.0 || proj.y > 1.0) return 1.0;
    float bias = max(0.001, 0.0005 * (1.0 - NdotL));
    float sum = 0.0;
    for (int y = -1; y <= 1; ++y)
      for (int x = -1; x <= 1; ++x) {
        vec2 off = vec2(float(x), float(y)) * u_ShadowTexelSize;
        sum += texture(u_ShadowMap, vec3(proj.xy + off, proj.z - bias));
      }
    return sum / 9.0;
}

void main() {
    vec3 N = normalize(v_Normal);
    vec3 V = normalize(u_CameraPos - v_FragPos);

    vec3 L;
    float atten = 1.0;
    float sf = 1.0;

    if (u_LightType == LIGHT_DIRECTIONAL) {
        L = -normalize(u_LightDir);
    } else if (u_LightType == LIGHT_POINT) {
        vec3 Lvec = u_LightPos - v_FragPos;
        float d = length(Lvec);
        L = Lvec / max(d, 1e-5);
        atten = attenuationDistance(d, u_LightRange);
    } else { // SPOT
        vec3 Lvec = u_LightPos - v_FragPos;
        float d = length(Lvec);
        L = Lvec / max(d, 1e-5);
        atten = attenuationDistance(d, u_LightRange);
        sf = spotFactor(L, u_LightDir, u_SpotInnerCos, u_SpotOuterCos);
    }

    float NdotL = max(dot(N, L), 0.0);

    // Diffuse
    vec3 albedo = u_BaseColor;
    vec3 diffuse = albedo * NdotL;

    // Spec (Blinn-Phong like)
    float shininess = mix(8.0, 256.0, 1.0 - clamp(u_Roughness, 0.0, 1.0));
    vec3  F0 = mix(vec3(0.04), albedo, clamp(u_Metallic, 0.0, 1.0));
    vec3  H = normalize(L + V);
    float NdotH = max(dot(N, H), 0.0);
    vec3  specular = F0 * pow(NdotH, shininess);

    float visibility = (u_LightType == LIGHT_DIRECTIONAL) ? computeShadow(NdotL) : 1.0;

    vec3 lightScale = u_LightColor * u_LightIntensity * atten * sf * visibility;
    vec3 color = (diffuse + specular) * lightScale;

    color += u_EmissiveColor * u_EmissiveStrength;

    FragColor = vec4(max(color, vec3(0.0)), 1.0);
}
