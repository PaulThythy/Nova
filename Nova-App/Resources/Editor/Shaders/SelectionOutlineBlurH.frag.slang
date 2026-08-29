// Separable Gaussian blur (horizontal). Blurs R (selection coverage) for the glow halo.
// No nova ParameterBlock — must not touch engine frame/lights UBOs after Scene.

struct PSIn {
    float2 v_UV;
};

struct SelectionBlurResources {
    Texture2D source;
    SamplerState sourceSampler;
};
ParameterBlock<SelectionBlurResources> outline;

static const float kWeights[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };

[shader("fragment")]
float4 main(PSIn input) : SV_Target0 {
    uint w = 1, h = 1;
    outline.source.GetDimensions(w, h);
    float2 texel = float2(1.0 / max(float(w), 1.0), 0.0);

    float result = outline.source.Sample(outline.sourceSampler, input.v_UV).r * kWeights[0];
    for (int i = 1; i < 5; ++i) {
        float weight = kWeights[i];
        float2 offset = texel * (float)(i * 2);
        result += outline.source.Sample(outline.sourceSampler, input.v_UV + offset).r * weight;
        result += outline.source.Sample(outline.sourceSampler, input.v_UV - offset).r * weight;
    }
    return float4(result, 0.0, 0.0, 1.0);
}