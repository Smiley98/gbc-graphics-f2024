#version 460 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTcoord;

uniform mat4 u_orbit;
uniform mat4 u_world[100];
uniform mat4 u_mvp;
uniform mat3 u_normal;

// Extra practice: output positions & normals, then apply lighting!
out vec2 tcoord;

mat4 translate(vec3 delta)
{
    return mat4(
        vec4(1.0, 0.0, 0.0, 0.0),
        vec4(0.0, 1.0, 0.0, 0.0),
        vec4(0.0, 0.0, 1.0, 0.0),
        vec4(delta, 1.0));
}

void main()
{
   int id = gl_InstanceID;
   mat4 world = u_world[id];
   tcoord = aTcoord;

   gl_Position = u_mvp * u_orbit * world * vec4(aPosition, 1.0);
}
