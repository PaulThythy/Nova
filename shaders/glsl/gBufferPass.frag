#version 330 core
in VS_OUT {
    vec3 worldPos;
    vec3 worldNrm;
} fs;

layout(location = 0) out vec4 oPosition;
layout(location = 1) out vec4 oNormal;
layout(location = 2) out vec4 oAlbedoRoughness;
layout(location = 3) out vec4 oMetallic;

uniform vec3  u_BaseColor;
uniform float u_Roughness;
uniform float u_Metallic;

void main() {
    oPosition = vec4(fs.worldPos, 1.0);

    vec3 n = normalize(fs.worldNrm);
    vec3 n01 = normalize(n) * 0.5 + 0.5;
    oNormal = vec4(n01, 1.0);

    oAlbedoRoughness = vec4(clamp(u_BaseColor, 0.0, 1.0), clamp(u_Roughness, 0.0, 1.0));

    oMetallic = vec4(clamp(u_Metallic, 0.0, 1.0), 0.0, 0.0, 0.0);
}