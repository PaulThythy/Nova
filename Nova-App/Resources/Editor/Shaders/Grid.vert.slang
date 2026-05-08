#include "NovaUniforms.slang"

// Fullscreen quad: two attributes at binding 0 — locations 0,1 (Slang field order).
struct VSIn {
    float2 position : POSITION;
    float2 uv : TEXCOORD0;
};

struct VSOut {
    float4 sv_position : SV_Position;
    float2 v_UV;
};

[shader("vertex")]
VSOut main(VSIn input) {
    VSOut o;
    o.sv_position = float4(input.position, 0.0, 1.0);
    o.v_UV        = input.uv;
    return o;
}
