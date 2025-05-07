#version 330 core
layout(location = 0) out vec4 gPosition;
layout(location = 1) out vec4 gNormal;


in vec3 FragPos;
in vec3 Normal;

void main()
{
    gPosition = vec4(FragPos, 1.0);
    
    gNormal = vec4(normalize(Normal),1.0);
    
}