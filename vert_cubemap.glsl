#version 330 core
layout (location = 0) in vec3 inVertex;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    TexCoords = inVertex;
    vec4 pos = projection * view * vec4(inVertex, 1.0);
    gl_Position = pos.xyww;
}