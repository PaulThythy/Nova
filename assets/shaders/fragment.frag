#version 330 core

in vec3 v_FragPos;
in vec3 v_Normal;

uniform vec3 u_CameraPos;

uniform vec3 u_LightPos;
uniform vec3 u_LightColor;
uniform float u_LightIntensity;

uniform vec3 u_ObjectColor;

out vec4 FragColor;

void main()
{
    // Normalized vectors
    vec3 normal = normalize(v_Normal);
    vec3 lightDir = normalize(u_LightPos - v_FragPos);
    vec3 viewDir = normalize(u_CameraPos - v_FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);

    // Components
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * u_LightColor;

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * u_LightColor;

    float specularStrength = 0.5;
    float shininess = 32.0;
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * u_LightColor;

    vec3 lighting = (ambient + diffuse + specular) * u_ObjectColor * u_LightIntensity;

    FragColor = vec4(lighting, 1.0);
}
