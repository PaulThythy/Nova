#version 330 core
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform float u_OutlineWorld;

void main()
{
    vec4 worldPos = u_Model * vec4(a_Position, 1.0);

    mat3 normalMat = mat3(transpose(inverse(u_Model)));
    vec3 worldN = normalize(normalMat * a_Normal);

    worldPos.xyz += worldN * u_OutlineWorld;

    gl_Position = u_Projection * u_View * worldPos;
}
