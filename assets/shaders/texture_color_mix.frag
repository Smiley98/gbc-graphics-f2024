#version 460 core

in vec2 tcoord;

uniform sampler2D u_tex0;
uniform sampler2D u_tex1;
uniform float u_t;

out vec4 FragColor;

void main()
{
    vec3 rgb0 = texture(u_tex0, tcoord).xyz;
    vec3 rgb1 = texture(u_tex1, tcoord).xyz;
    FragColor = vec4(mix(rgb0, rgb1, u_t), 1.0);
}
