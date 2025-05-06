#version 330 core

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;

out vec4 FragColor;

struct Light {
    vec3 position;
    vec3 color;
    float intensity;
};

const int NUM_LIGHTS = 2;
uniform vec3 viewPos;
uniform float exposure;

vec3 ambientColor = vec3(0.001, 0.001, 0.001);
vec3 albedo = vec3(0.8, 0.5, 0.3);

void main()
{
    Light lights[NUM_LIGHTS];
    lights[0] = Light(vec3(0.0, 15.0, 25.0), vec3(1.0, 1.0, 1.0), 1.0);
    lights[1] = Light(vec3(0.0, -15.0, 25.0), vec3(1.0, 1.0, 1.0), 1.0);

    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal = normalize(texture(gNormal, TexCoords).rgb);
    
    if(length(FragPos) == 0.0) {
        discard;
    }

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 lighting = ambientColor * albedo;
    
    for(int i = 0; i < NUM_LIGHTS; ++i)
    {
        vec3 lightDir = normalize(lights[i].position - FragPos);
        
        float diff = max(dot(Normal, lightDir), 0.0);
        vec3 diffuse = diff * lights[i].color * albedo * lights[i].intensity;
        
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(Normal, halfwayDir), 0.0), 32.0);
        vec3 specular = spec * lights[i].color * lights[i].intensity;
        
        float distance = length(lights[i].position - FragPos);
        float attenuation = 1.0 / (1.0 + 0.01 * distance + 0.01 * distance * distance);
        
        lighting += attenuation * (diffuse + specular);
    }

    vec3 result = lighting * exposure;
    
    FragColor = vec4(result, 1.0);
}
