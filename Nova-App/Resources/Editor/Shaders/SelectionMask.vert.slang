// Minimal MVP block only — do not reflect nova.frame / nova.scene / lights / shadows.
// Otherwise ApplyParameters would overwrite the shared frame/scene UBOs after Scene
// and the GPU would light the scene with stale/zero m_LightCount.

struct MVP {
    float4x4 m_Model;
    float4x4 m_View;
    float4x4 m_Proj;
    float4x4 m_ViewProj;
    float4x4 m_InvViewProj;
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
    float4 world = mul(nova.mvp.m_Model, float4(input.a_Position, 1.0));
    float4 viewPos = mul(nova.mvp.m_View, world);

    VSOut o;
    o.sv_position = mul(nova.mvp.m_Proj, viewPos);
    return o;
}