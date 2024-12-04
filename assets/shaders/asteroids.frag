#version 460 core

in vec2 tcoord;

uniform sampler2D u_tex;

out vec4 FragColor;

void main()
{
    vec3 col = texture(u_tex, tcoord).xyz;
    FragColor = vec4(col, 1.0);
}
