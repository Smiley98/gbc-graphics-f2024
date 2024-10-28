#version 460 core

// Attribute locations (5 * 13) chosen at random to illustrate that they're completely arbitrary (whatever numbers from 0-15 you want them to be)!
layout (location = 5) in vec2 aPosition;
layout (location = 13) in vec3 aColor;

out vec3 color;

void main()
{
   vec4 position = vec4(aPosition, 0.0, 1.0);
   color = aColor;
   gl_Position = position;
}
