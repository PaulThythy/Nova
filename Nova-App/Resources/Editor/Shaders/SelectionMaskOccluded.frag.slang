// Occluded selection (depth Greater): R = silhouette coverage, G = x-ray.
// Writing R=1 here fills holes where non-selected occluders sit on the
// selection (e.g. objects on a selected ground plane) so the blur halo only
// follows the true outer silhouette, not contact edges.
[shader("fragment")]
float4 main() : SV_Target0 {
    return float4(1.0, 1.0, 0.0, 1.0);
}