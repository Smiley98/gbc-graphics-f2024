#version 460 core

in vec3 position;

uniform samplerCube u_cubemap;

out vec4 FragColor;

void main()
{
    FragColor = texture(u_cubemap, position);
}
