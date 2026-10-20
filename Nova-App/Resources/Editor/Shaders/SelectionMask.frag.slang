// R = visible selection coverage (depth LessOrEqual). Clears G so self-occlusion
// x-ray from the occluded pass does not stick on the front surface.
[shader("fragment")]
float4 main() : SV_Target0 {
    return float4(1.0, 0.0, 0.0, 1.0);
}