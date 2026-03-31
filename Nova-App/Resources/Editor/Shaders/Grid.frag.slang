#include "NovaUniforms.slang"

struct PSIn {
    [[vk::location(0)]] float2 v_UV;
};

struct PSOut {
    float4 color : SV_Target0;
    float  depth : SV_Depth;
};

float gridLine(float2 coord, float step) {
    float2 r = coord / step;
    float2 g = abs(frac(r - 0.5) - 0.5) / fwidth(r);
    return 1.0 - min(min(g.x, g.y), 1.0);
}

[shader("fragment")]
PSOut main(PSIn input) {
    // Reconstruct world-space ray from screen UV via inverse view-projection.
    float2 ndc = input.v_UV * 2.0 - 1.0;

    float4 nearH = mul(nova.mvp.invViewProj, float4(ndc, 0.0, 1.0));
    float4 farH  = mul(nova.mvp.invViewProj, float4(ndc, 1.0, 1.0));
    float3 nearP = nearH.xyz / nearH.w;
    float3 farP  = farH.xyz  / farH.w;

    float3 dir = normalize(farP - nearP);

    // Intersect the Y=0 plane.
    float denom = dir.y;
    if (abs(denom) < 1e-6) discard;

    float t = -nearP.y / denom;
    if (t < 0.0) discard;

    float3 pos = nearP + dir * t;

    // Depth at the Y=0 intersection (bias toward camera so grid sits above a ground plane).
    float4 clip     = mul(nova.mvp.viewProj, float4(pos, 1.0));
    float  ndcDepth = clip.z / clip.w;

    // Distance fade.
    float dist = length(pos - nova.globals.u_CameraPos);
    float fade = 1.0 - clamp(dist / 50.0, 0.0, 1.0);

    float2 xz = pos.xz;

    // Grid lines.
    float small = gridLine(xz, 1.0);
    float major = gridLine(xz, 5.0);

    // Axis highlights (X red, Z blue).
    float axX = 1.0 - smoothstep(0.0, fwidth(xz.y) * 2.0, abs(xz.y));
    float axZ = 1.0 - smoothstep(0.0, fwidth(xz.x) * 2.0, abs(xz.x));

    float3 col = float3(0.0, 0.0, 0.0);
    float  a   = 0.0;

    col += float3(0.5, 0.5, 0.5) * (small * 0.35);
    a    = max(a, small * 0.35);

    col = lerp(col, float3(0.7, 0.7, 0.7), major);
    a   = max(a, major);

    col = lerp(col, float3(1.0, 0.2, 0.2), axX);   // X axis — red
    a   = max(a, axX);

    col = lerp(col, float3(0.2, 0.2, 1.0), axZ);   // Z axis — blue
    a   = max(a, axZ);

    a *= fade;
    if (a < 0.01) discard;

    PSOut o;
    o.color = float4(col, a);
    o.depth = clamp(ndcDepth - 1e-5, 0.0, 1.0);
    return o;
}
