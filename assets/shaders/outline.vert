#version 330 core
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform float u_OutlineWidth;

void main()
{
    vec3 inflated = a_Position + a_Normal * u_OutlineWidth;
    gl_Position = u_Projection * u_View * u_Model * vec4(inflated, 1.0);
}
