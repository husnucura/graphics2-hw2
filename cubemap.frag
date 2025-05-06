#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube cubeSampler;
uniform float exposure;
uniform float gamma;

void main()
{    
    vec3 hdrColor = texture(cubeSampler, TexCoords).rgb;
    vec3 mapped = hdrColor* exposure;
    FragColor = vec4(mapped, 1.0);
}