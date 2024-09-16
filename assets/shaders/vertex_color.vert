#version 460 core

layout (location = 0) in vec3 aPosition;

out vec3 color;

void main()
{
   color = aPosition * 0.5 + 0.5;
   gl_Position = vec4(aPosition, 1.0);
}
