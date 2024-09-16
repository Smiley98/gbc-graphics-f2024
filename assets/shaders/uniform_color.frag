#version 460 core

uniform vec3 u_color;
uniform float u_intensity;

out vec4 FragColor;

void main()
{
    FragColor = vec4(u_color * u_intensity, 1.0);
}
