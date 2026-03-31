#include "NovaUniforms.slang"

struct VSOut {
    float4 sv_position : SV_Position;
    [[vk::location(0)]] float2 v_UV;
};

[shader("vertex")]
VSOut main(uint vertexID : SV_VertexID) {
    float2 p;
    if (vertexID == 0u)      p = float2(-1.0, -1.0);
    else if (vertexID == 1u) p = float2( 3.0, -1.0);
    else                     p = float2(-1.0,  3.0);

    VSOut o;
    o.v_UV        = p * 0.5 + 0.5;
    o.sv_position = float4(p, 0.0, 1.0);
    return o;
}
