#version 460 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTcoord;

uniform mat4 u_mvp;
uniform mat4 u_world;
uniform mat3 u_normal;

out vec3 position;
out vec3 normal;
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

   position = (u_world * vec4(aPosition, 1.0)).xyz;
   normal = u_normal * aNormal;
   tcoord = aTcoord;

   mat4 t = translate(vec3(0.1 * float(id), 0.0, 0.0));
   gl_Position = u_mvp * t * vec4(aPosition, 1.0);
}
