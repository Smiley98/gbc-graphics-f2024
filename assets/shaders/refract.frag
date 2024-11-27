#version 460 core

in vec3 position;

uniform samplerCube u_cubemap;
uniform vec3 u_cameraPosition;
uniform float u_ratio;

out vec4 FragColor;

void main()
{
    vec3 col = texture(u_cubemap, position).xyz;
    FragColor = vec4(col, 1.0);
}
