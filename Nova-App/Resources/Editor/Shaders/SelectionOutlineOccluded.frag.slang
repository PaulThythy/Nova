[shader("fragment")]
float4 main() : SV_Target0 {
    // Dim yellow for occluded outline (behind ground / other meshes).
    return float4(1.0, 0.9, 0.15, 0.35);
}