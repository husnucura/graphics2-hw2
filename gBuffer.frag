#version 330 core
layout(location = 0) out vec3 FragPos;
layout(location = 1) out vec3 FragNormal;

in vec3 WorldPos;
in vec3 Normal;

void main()
{
    FragPos = normalize(WorldPos) + vec3(0.50);
    FragNormal = normalize(Normal);
}
