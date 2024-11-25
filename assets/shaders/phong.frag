#version 460 core

in vec3 position;
in vec3 normal;

out vec4 FragColor;

// TODO for yourself: figure out how to send more than 1 light worth of information to this shader
uniform vec3 u_cameraPosition;
uniform vec3 u_lightPosition;
uniform vec3 u_lightDirection;
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

// Phong but attenuated
vec3 point_light(vec3 position, vec3 normal, vec3 camera, vec3 light, vec3 color, float ambientFactor, float diffuseFactor, float specularPower, float radius)
{
    vec3 lighting = phong(position, normal, u_cameraPosition, u_lightPosition, u_lightColor, u_ambientFactor, u_diffuseFactor, u_specularPower);

    float dist = length(light - position);
    float attenuation = clamp(radius / dist, 0.0, 1.0);
    lighting *= attenuation;

    return lighting;
}

// Phong but based on direction only
vec3 direction_light(vec3 direction, vec3 normal, vec3 camera, vec3 color, float ambientFactor, float diffuseFactor, float specularPower)
{
    vec3 lighting = phong(vec3(0.0), normal, u_cameraPosition, -direction, u_lightColor, u_ambientFactor, u_diffuseFactor, u_specularPower);
    return lighting;
}

// Phong but attenuated & within field of view (fov)
vec3 spot_light(vec3 position, vec3 direction, vec3 normal, vec3 camera, vec3 light, vec3 color, float ambientFactor, float diffuseFactor, float specularPower, float radius, float fov)
{
    // TODO -- figure this out for yourself
    vec3 lighting = vec3(0.0);
    return lighting;
}

void main()
{
    //vec3 lighting = point_light(position, normal, u_cameraPosition, u_lightPosition, u_lightColor, u_ambientFactor, u_diffuseFactor, u_specularPower, u_lightRadius);
    vec3 lighting = direction_light(u_lightDirection, normal, u_cameraPosition, u_lightColor, u_ambientFactor, u_diffuseFactor, u_specularPower);
    // TODO -- test spot light and multiple lights
    FragColor = vec4(lighting, 1.0);
}

// Extra practice: Add more information to light such as ambient-diffuse-specular + intensity
// https://learnopengl.com/Lighting/Materials
// https://learnopengl.com/Lighting/Lighting-maps
