#version 330

//layout(location = 0) in vec2 pos;
layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 nor;

uniform mat4 mvp;

out vec3 nnn;

void main()                     
{
	nnn = vec3(nor.x, 0, nor.y);

	gl_Position = mvp * vec4(pos, 1.0);
}