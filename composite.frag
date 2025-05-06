#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D lightingTexture;

uniform sampler2D cubemapTexture2D;
uniform samplerCube cubemapTexture;

uniform vec3 viewPos;
uniform float reflectionStrength;

void main()
{
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal = normalize(texture(gNormal, TexCoords).rgb);
    
    vec3 lighting = texture(lightingTexture, TexCoords).rgb;
    
    if (length(FragPos) == 0.0) {

        FragColor = texture(cubemapTexture2D, TexCoords);
        return;
  
    }

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-viewDir, Normal);
    
    vec3 envColor;
    envColor = texture(cubemapTexture, reflectDir).rgb;
 
    
    vec3 finalColor = lighting + envColor * reflectionStrength;
    
    FragColor = vec4(finalColor, 1.0);
}