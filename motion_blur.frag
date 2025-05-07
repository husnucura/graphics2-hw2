#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D inputTexture;
uniform vec2 texelSize;
uniform float blurAmount;

vec2 direction = vec2(1.0, 1.0);

void main() {
    vec3 color;

    if (blurAmount <= 0.0) {
        color = texture(inputTexture, TexCoords).rgb;
    } else {
        vec2 offset = texelSize * blurAmount * 15.0 * direction;

        color = texture(inputTexture, TexCoords).rgb * 0.2270270270;
        color += texture(inputTexture, TexCoords + offset * 1.3846153846).rgb * 0.3162162162;
        color += texture(inputTexture, TexCoords - offset * 1.3846153846).rgb * 0.3162162162;
        color += texture(inputTexture, TexCoords + offset * 3.2307692308).rgb * 0.0702702703;
        color += texture(inputTexture, TexCoords - offset * 3.2307692308).rgb * 0.0702702703;
    }

    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    float logLum = log(max(luminance, 1e-6));
    FragColor = vec4(color, logLum);
}
