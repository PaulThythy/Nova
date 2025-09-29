#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform mat3 u_NormalMatrix;

out VS_OUT {
    vec3 worldPos;
    vec3 worldNrm;
} vs;

void main() {
    vec4 wp = u_Model * vec4(aPos, 1.0);
    vs.worldPos = wp.xyz;
    vs.worldNrm = normalize(u_NormalMatrix * aNormal);
    gl_Position = u_Projection * u_View * wp;
}
