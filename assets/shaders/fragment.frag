#version 330 core

in vec3 v_FragPos;
in vec3 v_Normal;

uniform vec3 u_CameraPos;

uniform vec3 u_LightPos;
uniform vec3 u_LightColor;
uniform float u_LightIntensity;

uniform vec3  u_BaseColor;
uniform float u_Roughness;
uniform float u_Metallic;
uniform vec3  u_EmissiveColor;
uniform float u_EmissiveStrength;

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

    vec3  F0 = mix(vec3(0.04), albedo, metal);

    float  D = D_GGX(NdotH, a);
    float  G = G_SmithGGXCorrelated(NdotV, NdotL, a);
    vec3   F = F_Schlick(F0, VdotH);

    vec3  specular = (D * G) * F / (4.0 * NdotV * NdotL + 1e-7);

    // Diffuse term (Lambertian, reduced by metallic)
    vec3 kd = (1.0 - F) * (1.0 - metal);
    vec3 diffuse = kd * albedo / 3.14159265;

    // Light color and intensity (no attenuation for now)
    vec3 light = u_LightColor * u_LightIntensity;
    vec3 radiance = light;

    // Combine diffuse and specular contributions
    vec3 color = (diffuse + specular) * radiance * NdotL;

    // Add emissive contribution
    color += u_EmissiveColor * u_EmissiveStrength;

    // Clamp to avoid NaN/inf values
    color = max(color, vec3(0.0));

    // (Optional) gamma correction
    // color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, 1.0);
}
