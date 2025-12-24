#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in vec3 a_Color;
layout(location = 4) in vec3 a_Tangent;
layout(location = 5) in vec3 a_Bitangent;

uniform mat4 u_ViewProjection;
uniform mat4 u_Model;

out vec3 v_Color;

void main()
{
    v_Color = a_Color;

    gl_Position = u_ViewProjection * u_Model * vec4(a_Position, 1.0);
}