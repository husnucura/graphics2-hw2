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

    if(visualizeMode == 0){
        FragColor = vec4(texValue,1.0);
        return;

    }
    if(length(texValue) == 0.0) {
        FragColor = vec4(texValue, 1.0);
        return;
    }
    

    if (visualizeMode == 1) {
        vec3 normalColor = texValue * 0.5 + 0.5;
        FragColor = vec4(normalColor, 1.0);
    }
    else {
       
        
    }
}
