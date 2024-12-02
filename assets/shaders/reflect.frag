#version 460 core

in vec3 position;
in vec3 normal;

uniform samplerCube u_cubemap;
uniform vec3 u_cameraPosition;

out vec4 FragColor;

void main()
{
    vec3 I = normalize(position - u_cameraPosition);
    vec3 R = reflect(I, normalize(normal));
    
    vec3 col = texture(u_cubemap, R).xyz;
    FragColor = vec4(col, 1.0);
}
