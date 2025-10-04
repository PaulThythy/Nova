#version 330 core
layout(location=0) in vec3 aPosWS;     // vertices in world space
uniform mat4 u_ViewProjection;         // VP = Projection * View
void main() {
    gl_Position = u_ViewProjection * vec4(aPosWS, 1.0);
}
