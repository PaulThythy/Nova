#include "NovaUniforms.slang"

struct VSIn {
    [[vk::location(0)]] float2 position : POSITION;
    [[vk::location(1)]] float2 uv : TEXCOORD0;
};

struct VSOut {
    float4 sv_position : SV_Position;
    [[vk::location(0)]] float2 v_UV;
};

[shader("vertex")]
VSOut main(VSIn input) {
    VSOut o;
    o.sv_position = float4(input.position, 0.0, 1.0);
    o.v_UV        = input.uv;
    return o;
}
