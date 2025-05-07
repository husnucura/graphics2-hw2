#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D inputTexture;
uniform float keyValue;
uniform float gammaValue;

uniform bool toneMappingEnabled;
uniform bool gammaEnabled;

vec3 toneMapReinhard(vec3 color, float key, float avgLum) {
    vec3 scaledColor = color * (key / max(avgLum,1e-5));
    return scaledColor / (vec3(1.0) + scaledColor);

}

void main() 
{
    vec4 hdrData = texture(inputTexture, TexCoords);
    vec3 hdrColor = hdrData.rgb;
    float logLum = hdrData.a;
    
    if (toneMappingEnabled) {
 
        float avgLogLum = textureLod(inputTexture, vec2(0.5), 100.0).a;
        float avgLum = exp(avgLogLum);
        avgLum = floor(avgLum * 1000.0 + 0.5) / 1000.0;
        
        hdrColor = toneMapReinhard(hdrColor, keyValue, avgLum);
        hdrColor = clamp(hdrColor, 0.0, 1.0);
    }
    
    if (gammaEnabled) {
        hdrColor = pow(hdrColor, vec3(1.0 / gammaValue));
    }
    
    FragColor = vec4(hdrColor, 1.0);
}