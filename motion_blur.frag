#version 330 core
in vec2 TexCoords;

layout(location = 0) out vec4 FragColor; 
layout(location = 1) out float luminanceOutput;

uniform sampler2D inputTexture;
uniform float blurAmount;
uniform vec2 texelSize;

void main() {
    vec4 original = texture(inputTexture, TexCoords);
    vec4 color = original;

    if (blurAmount > 0.0) {
        vec4 result = vec4(0.0);
        float total = 0.0;
        const int radius = 2;

        int samples = int(ceil(blurAmount * 50.0)); 
        samples = clamp(samples, 1, radius*2+1);

        for (int y = -radius; y <= radius; ++y) {
            for (int x = -radius; x <= radius; ++x) {
                if (abs(x) > samples/2 || abs(y) > samples/2) continue;
                vec2 offset = vec2(x, y) * texelSize * blurAmount * 10.0;
                vec2 sampleCoords = clamp(TexCoords + offset, vec2(0.0), vec2(1.0));
                result += texture(inputTexture, sampleCoords);
                total += 1.0;
            }
        }
        vec4 blurred = result / total;
        color = mix(blurred, original, 0.3);
    }

    float luminance = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
    float logLum = log(max(luminance, 1e-5));

    FragColor = color;
    luminanceOutput = logLum;
}