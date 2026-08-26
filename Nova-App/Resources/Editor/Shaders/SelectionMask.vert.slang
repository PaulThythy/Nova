// Minimal MVP block only — do not reflect nova.frame / lights / shadows.
// Otherwise ApplyParameters would overwrite the shared frame UBO after Scene
// and the GPU would light the scene with stale/zero lightCount.

struct MVP {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
    float4x4 viewProj;
    float4x4 invViewProj;
};

struct SelectionMaskEngine {
    ConstantBuffer<MVP> mvp;
};

ParameterBlock<SelectionMaskEngine> nova;

struct VSIn {
    float3 a_Position;
    float3 a_Normal;
    float2 a_TexCoord;
    float3 a_Color;
    float3 a_Tangent;
    float3 a_Bitangent;
};

struct VSOut {
    float4 sv_position : SV_Position;
};

[shader("vertex")]
VSOut main(VSIn input) {
    float4 world = mul(nova.mvp.model, float4(input.a_Position, 1.0));
    float4 viewPos = mul(nova.mvp.view, world);

    VSOut o;
    o.sv_position = mul(nova.mvp.proj, viewPos);
    return o;
}