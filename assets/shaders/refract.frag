#version 460 core

in vec3 position;
in vec3 normal;

uniform samplerCube u_cubemap;
uniform vec3 u_cameraPosition;
uniform float u_ratio;

out vec4 FragColor;

void main()
{
    vec3 I = normalize(position - u_cameraPosition);
    vec3 R = refract(I, normalize(normal), u_ratio);
    
    vec3 col = texture(u_cubemap, position).xyz;
    FragColor = vec4(col, 1.0);
}
