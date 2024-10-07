#version 460 core

in vec3 normal;

out vec4 FragColor;

void main()
{
    FragColor = vec4(normal, 1.0);
}
