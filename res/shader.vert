#version 330

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

smooth out vec3 outColor;

uniform mat4 mvpMatrix;

void main()
{
	gl_Position = mvpMatrix * vec4(position, 1.0f);
	outColor = color;
}
