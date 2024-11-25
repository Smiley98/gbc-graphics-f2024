#version 460 core

in vec3 position;
in vec3 normal;

out vec4 FragColor;

uniform vec3 u_cameraPosition;
uniform vec3 u_lightPosition;
uniform vec3 u_lightColor;
uniform float u_lightRadius;

uniform float u_ambientFactor;
uniform float u_diffuseFactor;
uniform float u_specularPower;

vec3 phong(vec3 position, vec3 normal, vec3 camera, vec3 light, vec3 color, float ambientFactor, float diffuseFactor, float specularPower)
{
    vec3 N = normalize(normal);
    vec3 L = normalize(light - position);
    vec3 V = normalize(camera - position);
    vec3 R = normalize(reflect(-L, N));
    float dotNL = max(dot(N, L), 0.0);
    float dotVR = max(dot(V, R), 0.0);

    vec3 lighting = vec3(0.0);
    vec3 ambient = color * ambientFactor;
    vec3 diffuse = color * dotNL * diffuseFactor;
    vec3 specular = color * pow(dotVR, specularPower);

    lighting += ambient;
    lighting += diffuse;
    lighting += specular;
    return lighting;
}



void main()
{
    // Whether its a point light, directional light, or spotlight, the underlying illumination model (phong) is the same!
    vec3 lighting = phong(position, normal, u_cameraPosition, u_lightPosition, u_lightColor, u_ambientFactor, u_diffuseFactor, u_specularPower);

    // Convert to point light by applying attenuation:    
    float dist = length(u_lightPosition - position);
    float attenuation = clamp(u_lightRadius / dist, 0.0, 1.0);
    lighting *= attenuation;

    FragColor = vec4(lighting, 1.0);
}

// Extra practice: Add more information to light such as ambient-diffuse-specular + intensity
// https://learnopengl.com/Lighting/Materials
// https://learnopengl.com/Lighting/Lighting-maps
