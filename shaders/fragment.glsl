#version 330 core
out vec4 FragColor;

in vec2 TexCoords; // receives UVs from vertex shader
in vec3 Normal;
in vec3 FragPos;

uniform vec3 objectColor;
uniform sampler2D mytexture;
uniform bool hasTexture;
uniform vec3 viewPos;
uniform float globalAmbientStrength;
uniform float spotlightAmbientStrength;
uniform float headlightAmbientStrength;
uniform vec3 sunDirection;
uniform vec3 sunColor;
uniform float sunAmbientStrength;
uniform float sunDiffuseStrength;
uniform float sunSpecularStrength;

// shininess and strength of metal
uniform float shininess;
uniform float specularStrength;

struct SpotLight {
    vec3 position;
    vec3 direction;
    vec3 color;
    
    float cutOff;
    float outerCutOff;
    
    // Attenuation
    float constant;
    float linear;
    float quadratic;
};

#define MAX_BUILDING_LIGHTS 10
uniform SpotLight buildingLights[MAX_BUILDING_LIGHTS];
uniform int numBuildingLights;

#define MAX_CAR_HEADLIGHTS 2
uniform SpotLight carHeadlights[MAX_CAR_HEADLIGHTS];
uniform int numCarHeadlights;

// Function for calculating spotlight
vec3 CalcSpotLight(SpotLight light,
                   vec3 normal,
                   vec3 fragPos,
                   vec3 viewDir,
                   vec3 albedoColor,
                   float ambientStrength){
    vec3 lightDir = normalize(light.position - fragPos);
    
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    
    // Specular (Blinn-Phong is usually better for metallic surfaces)
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    
    // Attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    
    // Spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction)); 
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    
    vec3 ambient = light.color * ambientStrength * albedoColor;
    vec3 diffuse = light.color * diff * albedoColor;
    vec3 specular = light.color * spec * specularStrength; // White specular highlights for metallic feel
    
    return (ambient + diffuse + specular) * attenuation * intensity;
}

vec3 CalcSunLight(vec3 normal,
                  vec3 viewDir,
                  vec3 albedoColor) {
    vec3 lightDir = normalize(-sunDirection);

    float diff = max(dot(normal, lightDir), 0.0);

    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);

    vec3 ambient = sunColor * sunAmbientStrength * albedoColor;
    vec3 diffuse = sunColor * (sunDiffuseStrength * diff) * albedoColor;
    vec3 specular = sunColor * (sunSpecularStrength * spec) * specularStrength;

    return ambient + diffuse + specular;
}

void main()
{
    vec3 albedo = hasTexture ? texture(mytexture, TexCoords).rgb * objectColor : objectColor;
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    vec3 result = albedo * globalAmbientStrength;
    result += CalcSunLight(norm, viewDir, albedo);
    
    for(int i = 0; i < numBuildingLights; i++) {
        result += CalcSpotLight(buildingLights[i], norm, FragPos, viewDir, albedo, spotlightAmbientStrength);
    }

    for(int i = 0; i < numCarHeadlights; i++) {
        result += CalcSpotLight(carHeadlights[i], norm, FragPos, viewDir, albedo, headlightAmbientStrength);
    }
    
    FragColor = vec4(result, 1.0);
}