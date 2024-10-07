#version 460 core

in vec2 tcoord;

out vec4 FragColor;

void main()
{
    FragColor = vec4(tcoord, 0.0, 1.0);
}
