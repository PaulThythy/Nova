// Additive yellow halo + soft occluded x-ray fill. No nova ParameterBlock.

struct PSIn {
    float2 v_UV;
};

struct SelectionOutlineResources {
    Texture2D selectionMask;
    SamplerState selectionMaskSampler;
    Texture2D selectionBlurred;
    SamplerState selectionBlurredSampler;
};
ParameterBlock<SelectionOutlineResources> outline;

static const float3 kOutlineColor = float3(1.0, 0.9, 0.15);
static const float kHaloStrength = 2.25;
static const float kOccludedStrength = 0.22;

[shader("fragment")]
float4 main(PSIn input) : SV_Target0 {
    float2 maskSample = outline.selectionMask.Sample(outline.selectionMaskSampler, input.v_UV).rg;
    float coverage = maskSample.r;
    float occluded = maskSample.g;
    float blurred = outline.selectionBlurred.Sample(outline.selectionBlurredSampler, input.v_UV).r;

    // Soft outer glow only (keep the blurred look, no hard edge).
    float halo = max(blurred - coverage, 0.0) * kHaloStrength;
    float intensity = saturate(halo + occluded * kOccludedStrength);
    if (intensity < 0.004)
        discard;

    // Premultiplied RGB for additive ONE/ONE blending — adds yellow, never darkens.
    return float4(kOutlineColor * intensity, 1.0);
}
