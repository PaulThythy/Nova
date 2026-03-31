#include "NovaUniforms.slang"

struct PSIn {
    [[vk::location(0)]] float2 v_UV;
};

struct PSOut {
    float4 color : SV_Target0;
    float  depth : SV_Depth;
};

static const float GridSize = 1.0;
static const uint  GridMinorsPerMajor = 4;

float fogFade(float viewDist, float nearDist, float farDist) {
    return 1.0 - smoothstep(nearDist, farDist, viewDist);
}

float fogScaledFade(float viewDist, float nearDist, float farDist, float2 fogScale) {
    return 1.0 - smoothstep(nearDist * fogScale.x, farDist * fogScale.y, viewDist);
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

    // View-space distance for fog.
    float3 viewPos  = mul(nova.mvp.view, float4(pos, 1.0)).xyz;
    float  viewDist = length(viewPos);

    // Fog tuning (world units in view-space).
    // Major lines persist longer; minor lines fade earlier.
    float majorFog = fogFade(viewDist, 10.0, 120.0);
    float minorFog = fogScaledFade(viewDist, 10.0, 120.0, float2(0.5, 0.85));

    // Grid coordinates pinned to world-space.
    float2 gridCoord = pos.xz / GridSize + 0.5;

    // Closest line index in each axis.
    int2 lineIndex = (int2)floor(gridCoord);

    // Choose width/color/fog per nearest X- and Z-bounded line.
    float2 lineWidth;
    float3 lineColor[2];
    float2 lineFog;
    for (int i = 0; i < 2; i++) {
        float width;
        float3 color;
        float fog;

        if (lineIndex[i] == 0) {
            width = 5.0;
            // Axis colors: X axis = red, Z axis = blue.
            // `i` indexes the bounded coordinate (0=x, 1=z), so swap for axis identity.
            color = (i == 0) ? float3(0.2, 0.2, 1.0) : float3(1.0, 0.2, 0.2);
            fog = majorFog;
        }
        else if ((lineIndex[i] % (int)GridMinorsPerMajor) == 0) {
            width = 3.0;
            color = float3(0.55, 0.55, 0.55);
            fog = majorFog;
        }
        else {
            width = 2.0;
            color = float3(0.25, 0.25, 0.25);
            fog = minorFog;
        }

        lineWidth[i] = width;
        lineColor[i] = color;
        lineFog[i]   = fog;
    }

    // Distance from the center of the nearest line, for both axes.
    float2 lineDist = abs(0.5 - frac(gridCoord)) * 2.0;
    float2 lineMask = 1.0 - saturate(lineDist / (fwidth(gridCoord) * lineWidth));

    // Base blend factors.
    float2 blendFactors = lineMask * lineFog;

    // Prevent gray gridlines from overpowering axes near the origin.
    for (int i = 0; i < 2; i++)
        if (lineIndex[1 - i] == 0 && lineIndex[i] != 0)
            blendFactors[i] *= smoothstep(0.0, 0.5, lineDist[1 - i]);

    float3 finalColor = max(lineColor[0] * blendFactors.x, lineColor[1] * blendFactors.y);
    float  alpha      = max(blendFactors.x, blendFactors.y);

    if (alpha < 0.01) discard;

    PSOut o;
    o.color = float4(finalColor, alpha);
    o.depth = clamp(ndcDepth - 1e-5, 0.0, 1.0);
    return o;
}
