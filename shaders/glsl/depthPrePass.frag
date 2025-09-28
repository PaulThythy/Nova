#version 330 core
layout(location=0) out vec4 oColor;

uniform float u_Near;
uniform float u_Far;

// Classic linearization from gl_FragCoord.z (NDC -> depth buffer)
float LinearizeDepth(float depth) {
    // depth here is in [0,1] (after division by w and remapping)
    float z = depth * 2.0 - 1.0;      // back to NDC (-1..1)
    float n = u_Near;
    float f = u_Far;
    // z_eye = (2n f) / (f + n - z (f - n))
    float zEye = (2.0 * n * f) / (f + n - z * (f - n));
    // Normalize 0..1 for display
    return (zEye - n) / (f - n);
}

void main() {
    float depth = LinearizeDepth(gl_FragCoord.z);
    oColor = vec4(vec3(depth), 1.0);
}