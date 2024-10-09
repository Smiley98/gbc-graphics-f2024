#version 460 core

// Position attribute is now a vec2 (see glVertexAttribPointer parameters)
layout (location = 0) in vec2 aPosition;

out vec3 color;
uniform float u_a;

void main()
{
   //color = vec3(aPosition * 0.5 + 0.5, u_a);
   color = vec3(aPosition * 0.5 + 0.5, 0.0);
   gl_Position = vec4(aPosition.xy, 0.0, 1.0);
}
