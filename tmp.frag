#version 330 core
in vec3 FragPos;
out vec4 FragColor;



void main()
{
    FragColor = vec4(clamp(FragPos-vec3(-1.2,-1,-6 -0.9)/2, 0.0, 1.0), 1.0);
}
