// Selection mask: camera from shared scene UBO + world matrix from nova.instances.
// Uses a minimal ParameterBlock so ApplyParameters does not require the full NovaEngine.

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
    int m_LightIndex;
};

struct InstanceGPU {
    float4x4 m_Model;
    uint m_MaterialIndex;
    uint _pad0;
    uint _pad1;
    uint _pad2;
};

struct SceneParameterBlock {
    ConstantBuffer<SceneUniforms> uniforms;
};

struct SelectionMaskEngine {
    SceneParameterBlock scene;
    StructuredBuffer<InstanceGPU> instances;
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
VSOut main(VSIn input, uint instanceIndex : SV_VulkanInstanceID) {
    InstanceGPU instance = nova.instances[instanceIndex];
    float4 world = mul(instance.m_Model, float4(input.a_Position, 1.0));
    float4 viewPos = mul(nova.scene.uniforms.m_Camera.m_View, world);

    VSOut o;
    o.sv_position = mul(nova.scene.uniforms.m_Camera.m_Proj, viewPos);
    return o;
}