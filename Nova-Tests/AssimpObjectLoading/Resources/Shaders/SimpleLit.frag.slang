#include "NovaUniforms.slang"

// Varyings must match SimpleLit.vert.slang VSOut (excluding SV_Position).
struct PSIn {
    float3 v_Normal;
    float3 v_Pos;
    float4 v_Color;
    nointerpolation uint v_MaterialIndex;
};

[shader("fragment")]
float4 main(PSIn input) : SV_Target0 {
    MaterialGPU material = nova.materials[input.v_MaterialIndex];
    float3 N = normalize(input.v_Normal);
    float3 base = material.m_Base * material.m_BaseColor;

    // Hardcoded directional light (world space).
    const float3 lightDir = normalize(float3(0.45, 0.85, 0.35));
    const float3 lightColor = float3(1.0, 0.97, 0.92);
    const float lightIntensity = 1.15;

    float ndotl = saturate(dot(N, lightDir));
    float3 diffuse = base * lightColor * (lightIntensity * ndotl);
    float3 ambient = base * 0.18;

    // Soft rim from camera for a bit of shape read.
    float3 V = normalize(nova.scene.uniforms.m_Camera.m_Position - input.v_Pos);
    float rim = pow(saturate(1.0 - saturate(dot(N, V))), 2.0) * 0.12;

    float3 color = ambient + diffuse + rim;
    return float4(color, 1.0);
}