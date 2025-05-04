#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D screenTexture;
uniform float blurAmount; 
uniform vec2 texelSize;    

void main()
{
    vec4 result = vec4(0.0);
    float total = 0.0;
    const int radius = 2;
    
    int samples = int(ceil(blurAmount * 50.0)); 
    samples = clamp(samples, 1, radius*2+1);
    
    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            if (abs(x) > samples/2 || abs(y) > samples/2) continue;
            
            vec2 offset = vec2(x, y) * texelSize * blurAmount * 10.0;
            vec2 sampleCoords = TexCoords + offset;
            
            
            // Method 1: Clamp to edges (fastest)
            sampleCoords = clamp(sampleCoords, vec2(0.0), vec2(1.0));
            
            // Method 2: Mirror sampling (better for tiling textures)
            // sampleCoords = abs(fract(sampleCoords * 0.5) * 2.0 - 1.0);
            
            // Method 3: Border color (replace vec4(0.0) with your border color)
            // if (any(lessThan(sampleCoords, vec2(0.0))) || 
            //     any(greaterThan(sampleCoords, vec2(1.0)))) {
            //     result += vec4(0.0);
            // } else {
            //     result += texture(screenTexture, sampleCoords);
            // }
            
            result += texture(screenTexture, sampleCoords);
            total += 1.0;
        }
    }

    // Normalize and apply slight contrast preservation
    vec4 blurred = result / total;
    vec4 original = texture(screenTexture, TexCoords);
    FragColor = mix(blurred, original, 0.3);  // 30% original sharpness
}