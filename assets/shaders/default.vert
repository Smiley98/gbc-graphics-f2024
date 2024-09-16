#version 460 core

layout (location = 0) in vec3 aPosition;

uniform mat4 u_world;

void main()
{
   vec4 position = u_world * vec4(aPosition, 1.0);
   gl_Position = position;
}
