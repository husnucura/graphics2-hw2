#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube cubeSampler;
uniform float exposure;
uniform float gamma;

void main()
{    
    vec3 hdrColor = texture(cubeSampler, TexCoords).rgb;
    
    vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);
    
    mapped = pow(mapped, vec3(1.0/gamma));
    
    FragColor = vec4(mapped, 1.0);
}