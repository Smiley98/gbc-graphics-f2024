#version 460 core

in vec3 position;
in vec3 normal;

out vec4 FragColor;

uniform vec3 u_lightPosition;
uniform vec3 u_lightColor;
uniform float u_lightRadius;

void main()
{
    // Extra practice: add ambient and specular components based on LearnOpenGL Basic Lighting article!!! 
    vec3 N = normalize(normal);
    vec3 L = normalize(u_lightPosition - position);
    float dotNL = max(dot(N, L), 0.0);

    // Visualize normals to understand their relation to lighting
    //vec3 diffuse = N;

    // Distance between light & fragment
    float dist = length(u_lightPosition - position);
    float attenuation = clamp(u_lightRadius / dist, 0.0, 1.0);

    // Extra practice 2: Apply different attenuation models via imgui or keyboard:
    // See the smoothstep function, makes for higher contrast between bright and dark
    // https://easings.net/#easeInOutSine
    //float attenuation = smoothstep(0.0, 1.0, u_lightRadius / dist);
    
    // Extra practice 3: Add more information to light such as ambient-diffuse-specular + intensity
    // https://learnopengl.com/Lighting/Materials
    // https://learnopengl.com/Lighting/Lighting-maps

    vec3 diffuse = u_lightColor * dotNL;
    diffuse *= attenuation;

    FragColor = vec4(diffuse, 1.0);
}
