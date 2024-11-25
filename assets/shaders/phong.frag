#version 460 core

in vec3 position;
in vec3 normal;

out vec4 FragColor;

uniform vec3 u_cameraPosition;
uniform vec3 u_lightPosition;
uniform vec3 u_lightColor;
uniform float u_lightRadius;

void main()
{
    vec3 N = normalize(normal);
    vec3 L = normalize(u_lightPosition - position);
    vec3 V = normalize(u_cameraPosition - position);
    vec3 R = normalize(reflect(L, N));
    float dotNL = max(dot(N, L), 0.0);
    float dotVR = max(dot(V, R), 0.0);

    // Distance between light & fragment
    float dist = length(u_lightPosition - position);
    float attenuation = clamp(u_lightRadius / dist, 0.0, 1.0);

    vec3 lighting = vec3(0.0);
    vec3 ambient = u_lightColor * 0.1;  // ambient value of 0.1, increase to make entire object more illuminated
    vec3 diffuse = u_lightColor * dotNL;
    vec3 specular = u_lightColor * pow(dotVR, 64);

    lighting += ambient;
    lighting += diffuse;
    lighting += specular;
    lighting *= attenuation;

    FragColor = vec4(lighting, 1.0);
}

// Extra practice: add ambient and specular components based on LearnOpenGL Basic Lighting article!!!
// Extra practice 2: Apply different attenuation models via imgui or keyboard:
// See the smoothstep function, makes for higher contrast between bright and dark
// https://easings.net/#easeInOutSine
// float attenuation = smoothstep(0.0, 1.0, u_lightRadius / dist);

// Extra practice 3: Add more information to light such as ambient-diffuse-specular + intensity
// https://learnopengl.com/Lighting/Materials
// https://learnopengl.com/Lighting/Lighting-maps