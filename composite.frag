#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D gPosition;      // Position buffer from geometry pass
uniform sampler2D gNormal;        // Normal buffer from geometry pass
uniform sampler2D lightingTexture; // Deferred lighting result
uniform samplerCube cubemapTexture; // Environment cubemap
uniform vec3 viewPos;             // Camera position
uniform vec3 cameraFront;
uniform float exposure;           // Exposure control
uniform float reflectionStrength; // Control reflection intensity (0.0-1.0)

void main()
{
    // Get position and normal from G-buffer
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal = texture(gNormal, TexCoords).rgb;
    
    if(length(FragPos) == 0.0) {
        discard;
        vec2 uv = TexCoords * 2.0 - 1.0;
        
        vec3 rayDir = normalize(vec3(uv.x, uv.y, 1.0)); 
        vec3 cameraRight = normalize(cross(cameraFront, vec3(0.0, 1.0, 0.0)));
        vec3 cameraUp = normalize(cross(cameraRight, cameraFront));
        vec3 worldDir = rayDir.x * cameraRight + rayDir.y * cameraUp + rayDir.z * cameraFront;
        
        vec3 envColor = texture(cubemapTexture, worldDir).rgb;
        envColor = vec3(1.0) - exp(-envColor * exposure);
        
        FragColor = vec4(envColor, 1.0);
        return;
    }
    // Sample deferred lighting result
    vec3 lighting = texture(lightingTexture, TexCoords).rgb;
    
    // Calculate reflection vector for environment sampling
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-viewDir, normalize(Normal));
    
    // Sample cubemap with reflection vector
    vec3 envColor = texture(cubemapTexture, reflectDir).rgb;
    
    // Apply exposure to both components
    lighting = vec3(1.0) - exp(-lighting * exposure);
    envColor = vec3(1.0) - exp(-envColor * exposure);
    
    // Combine lighting with environment reflection
    vec3 finalColor = lighting + (envColor * reflectionStrength);
    
    // Output final color
    FragColor = vec4(finalColor, 1.0);
}