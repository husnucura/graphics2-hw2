#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D screenTexture;
uniform int visualizeMode;
uniform vec3 minPos;
uniform vec3 maxPos;

void main()
{
    vec3 texValue = texture(screenTexture, TexCoords).rgb;

    

    if(length(texValue) == 0.0) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    if (visualizeMode == 1) {
        vec3 normalColor = texValue * 0.5 + 0.5;
        FragColor = vec4(normalColor, 1.0);
    }
    else {
        vec3 minBound = vec3(-1,-1.3,-7.5);
        vec3 maxBound = vec3(1.4,2.5,-4.5);
        vec3 normalizedPos = (texValue - minPos) / (maxPos - minPos);
        FragColor = vec4(clamp(normalizedPos, 0.0, 1.0), 1.0);
        //FragColor = vec4(0.5,0.0,0.0,1.0);
        
    }
}
