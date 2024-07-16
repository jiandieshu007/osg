#version 330 core

layout (location = 0) in vec3 Vertex;
layout (location = 1) in vec2 TexCoord;

out vec2 v_textureCoordinates;

void main()
{
    v_textureCoordinates = TexCoord;
    gl_Position = vec4(Vertex, 1.0);
}