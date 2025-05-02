#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D screenTexture;

void main()
{
    vec3 texColor = texture(screenTexture, TexCoords).rgb;
    FragColor = vec4(texColor, 1.0);
}
