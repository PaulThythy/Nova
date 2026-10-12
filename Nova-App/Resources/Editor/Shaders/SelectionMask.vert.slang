// Minimal scene.uniforms + model block. Binds the shared scene UBO (camera) uploaded by
// PushSceneUniforms — ApplyParameters does not rewrite scene uniforms, so this pass
// cannot clobber m_LightCount for later draws.

struct CameraGPU {
    float4x4 m_View;
    float4x4 m_Proj;
    float4x4 m_ViewProj;
    float4x4 m_InvViewProj;
    float3 m_Position;
    float _pad;
};

struct SceneUniforms {
    CameraGPU m_Camera;
    int m_LightCount;
    float _padAfterLightCount;
};

struct ModelUniforms {
    float4x4 m_Model;
    int m_LightIndex;
    float3 _padLightIndex;
};

struct SceneParameterBlock {
    ConstantBuffer<SceneUniforms> uniforms;
};

struct SelectionMaskEngine {
    SceneParameterBlock scene;
    ConstantBuffer<ModelUniforms> model;
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
    float4 world = mul(nova.model.m_Model, float4(input.a_Position, 1.0));
    float4 viewPos = mul(nova.scene.uniforms.m_Camera.m_View, world);

    VSOut o;
    o.sv_position = mul(nova.scene.uniforms.m_Camera.m_Proj, viewPos);
    return o;
}