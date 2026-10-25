// Visible selection coverage (depth LessOrEqual): R = 1, G = 0.
// Clears G so self-occlusion from the occluded pass does not leave a yellow
// x-ray fill on the front surface. Together with the occluded pass (which also
// writes R), channel R is a depth-agnostic full silhouette for the blur halo.
[shader("fragment")]
float4 main() : SV_Target0 {
    return float4(1.0, 0.0, 0.0, 1.0);
}