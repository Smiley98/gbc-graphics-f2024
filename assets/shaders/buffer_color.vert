#version 460 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aColor;

out vec3 color;

uniform mat4 u_mvp;

void main()
{
   color = aColor;
   vec4 position = u_mvp * vec4(aPosition, 1.0);
   gl_Position = position;
}
