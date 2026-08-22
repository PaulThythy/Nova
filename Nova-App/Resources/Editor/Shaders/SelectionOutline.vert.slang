#include "NovaUniforms.slang"

// Selection outline (inverted hull + Front cull).
//
// Do NOT extrude along shading normals: hard-edged meshes (cube) duplicate
// vertices per face, so adjacent faces move apart and leave gaps at edges.
// Instead, expand in local space away from the object origin — covers corners.

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

static const float kOutlineWidth = 0.05;

[shader("vertex")]
VSOut main(VSIn input) {
    float3 localPos = input.a_Position;
    float len = length(localPos);
    // Radial expand; fall back to normal if the vertex sits on the origin.
    float3 expandDir = (len > 1e-5) ? (localPos / len) : normalize(input.a_Normal);
    localPos += expandDir * kOutlineWidth;

    float4 world = mul(nova.mvp.model, float4(localPos, 1.0));

    VSOut o;
    float4 viewPos = mul(nova.mvp.view, world);
    o.sv_position = mul(nova.mvp.proj, viewPos);
    return o;
}