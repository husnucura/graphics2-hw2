#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D inputTexture;
uniform float blurAmount;
uniform vec2 texelSize;
uniform float exposure;

const float weights[7] = float[](0.01994, 0.05498, 0.12384, 0.19704, 0.22313, 0.19704, 0.12384);

void main() {
    vec4 original = texture(inputTexture, TexCoords);
    vec4 color = original;

    if (blurAmount > 0.0) {
        vec4 result = vec4(0.0);
        float totalWeight = 0.0;
        const int radius = 3;
        
        float strength = blurAmount * 5.0;
        
        for (int y = -radius; y <= radius; ++y) {
            for (int x = -radius; x <= radius; ++x) {
                float weight = weights[x + radius] * weights[y + radius];
                
                vec2 offset = vec2(x, y) * texelSize * strength;
                vec2 sampleCoords = clamp(TexCoords + offset, vec2(0.001), vec2(0.999));
                
                result += texture(inputTexture, sampleCoords) * weight;
                totalWeight += weight;
            }
        }
        
        vec4 blurred = result / totalWeight;
        color = mix(blurred, original, 0.1);
    }

    float luminance = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
    float logLum = log(max(luminance, 1e-6));
    FragColor = vec4(color.rgb, logLum);
}
