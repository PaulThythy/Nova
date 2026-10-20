// G only = occluded (depth Greater). Leave R at 0 so composite can gate x-ray
// with (1 - coverage) and keep the fill behind other objects only.
[shader("fragment")]
float4 main() : SV_Target0 {
    return float4(0.0, 1.0, 0.0, 1.0);
}