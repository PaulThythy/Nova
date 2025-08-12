#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform mat4 u_LightViewProj;

out vec3 v_FragPos;
out vec3 v_Normal;
out vec4 v_LightClip;

void main()
{
    vec4 worldPos = u_Model * vec4(a_Position, 1.0);
    v_FragPos = vec3(worldPos);
    v_Normal  = mat3(transpose(inverse(u_Model))) * a_Normal;
    v_LightClip = u_LightViewProj * worldPos;

    gl_Position = u_Projection * u_View * worldPos;
}
