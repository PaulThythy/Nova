#include "NovaUniforms.slang"

// Must match Grid.vert.slang VSOut (after SV_Position).
struct PSIn {
    float2 v_UV;
};

struct PSOut {
    float4 color : SV_Target0;
    float  depth : SV_Depth;
};

static const float GridSize = 1.0;
static const uint  GridMinorsPerMajor = 5;

float fogFade(float viewDist, float nearDist, float farDist) {
    return 1.0 - smoothstep(nearDist, farDist, viewDist);
}

float fogScaledFade(float viewDist, float nearDist, float farDist, float2 fogScale) {
    return 1.0 - smoothstep(nearDist * fogScale.x, farDist * fogScale.y, viewDist);
}

[shader("fragment")]
PSOut main(PSIn input)
{
    //----------------------------------------------------------
    // Reconstruct world-space ray from screen UV
    //----------------------------------------------------------

    float2 ndc = input.v_UV * 2.0 - 1.0;

    float4 nearH = mul(nova.mvp.m_InvViewProj, float4(ndc, 0.0, 1.0));

    float4 farH = mul(nova.mvp.m_InvViewProj, float4(ndc, 1.0, 1.0));

    float3 nearP = nearH.xyz / nearH.w;
    float3 farP = farH.xyz / farH.w;

    float3 dir = normalize(farP - nearP);

    //----------------------------------------------------------
    // Ray / Y=0 plane intersection
    //----------------------------------------------------------

    float denom = dir.y;

    if (abs(denom) < 1e-6)
        discard;

    float t = -nearP.y / denom;

    if (t < 0.0)
        discard;

    float3 pos = nearP + dir * t;

    //----------------------------------------------------------
    // Compute depth
    //----------------------------------------------------------

    float4 clip = mul(nova.mvp.m_ViewProj, float4(pos, 1.0));

    float ndcDepth = clip.z / clip.w;

    //----------------------------------------------------------
    // Compute distance from camera for fog
    //----------------------------------------------------------

    float3 viewPos = mul(nova.mvp.m_View, float4(pos, 1.0)).xyz;

    float viewDist = length(viewPos);

    float majorFog = fogFade(viewDist, 10.0, 120.0);

    float minorFog = fogScaledFade(viewDist, 10.0, 120.0, float2(0.5, 0.85));

    //----------------------------------------------------------
    // World-space grid coordinates
    //
    // No arbitrary +0.5 offset here.
    // Origin stays exactly at (0,0).
    //----------------------------------------------------------

    float2 gridCoord = pos.xz / GridSize;

    //----------------------------------------------------------
    // Determine nearest line index
    //
    // round() gives the closest grid line.
    //----------------------------------------------------------

    int2 lineIndex = (int2)round(gridCoord);

    float2 lineWidth;
    float3 lineColor[2];
    float2 lineFog;

    //----------------------------------------------------------
    // Configure line appearance
    //----------------------------------------------------------

    for (int i = 0; i < 2; i++) {
        float width;
        float3 color;
        float fog;

        // Axis lines (X=0 or Z=0)
        if (lineIndex[i] == 0)
        {
            width = 5.0;

            // Swap indices because:
            // i=0 -> X-bounded line = Z axis
            // i=1 -> Z-bounded line = X axis
            color = (i == 0) ? float3(0.2, 0.2, 1.0) : float3(1.0, 0.2, 0.2);

            fog = majorFog;
        }
        // Major grid lines
        else if ((abs(lineIndex[i]) % int(GridMinorsPerMajor)) == 0)
        {
            width = 3.0;
            color = float3(0.55, 0.55, 0.55);
            fog = majorFog;
        }
        // Minor grid lines
        else
        {
            width = 2.0;
            color = float3(0.25, 0.25, 0.25);
            fog = minorFog;
        }

        lineWidth[i] = width;
        lineColor[i] = color;
        lineFog[i] = fog;
    }

    //----------------------------------------------------------
    // Distance to nearest line
    //
    // Since lineIndex uses round(),
    // the nearest line is exactly at round(gridCoord).
    //----------------------------------------------------------

    float2 lineDist = abs(gridCoord - round(gridCoord));

    //----------------------------------------------------------
    // Anti-aliasing
    //----------------------------------------------------------

    float2 lineMask = 1.0 - saturate(lineDist / (fwidth(gridCoord) * lineWidth));

    float2 blendFactors = lineMask * lineFog;

    //----------------------------------------------------------
    // Prevent major/minor lines from bleeding into axes
    //----------------------------------------------------------

    for (int i = 0; i < 2; i++) {
        if (lineIndex[1 - i] == 0 && lineIndex[i] != 0) {
            blendFactors[i] *= smoothstep(0.0, 0.5, lineDist[1 - i]);
        }
    }

    //----------------------------------------------------------
    // Final color
    //----------------------------------------------------------

    float3 finalColor = max(lineColor[0] * blendFactors.x, lineColor[1] * blendFactors.y);

    float alpha = max(blendFactors.x, blendFactors.y);

    if (alpha < 0.01)
        discard;

    PSOut o;

    o.color = float4(finalColor, alpha);

    // Small bias so grid stays slightly above ground
    o.depth = clamp(ndcDepth - 1e-5, 0.0, 1.0);

    return o;
}
