#version 460 core

in vec2 tcoord;

uniform sampler2D u_tex;

out vec4 FragColor;

void main()
{
    FragColor = texture(u_tex, tcoord);
}
