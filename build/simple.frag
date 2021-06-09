#version 330 core

uniform vec3 objectColor;
uniform vec3 lightSource1;
uniform vec3 lightColor1;
uniform vec3 lightSource2;
uniform vec3 lightColor2;
uniform vec3 viewPos;

in vec3 FragPos;  
in vec3 normal;

out vec4 outColor;

float ambientStrength = 0.1;

float specularStrength = 10;
float shininess = 32;

void main()
{
    vec3 norm = normalize(normal);
    vec3 lightDir1 = normalize(lightSource1 - FragPos);  
    vec3 lightDir2 = normalize(lightSource2 - FragPos);  

    float diff1 = max(dot(norm, lightDir1), 0.0);
    float diff2 = max(dot(norm, lightDir2), 0.0);
    vec3 diffuse1 = diff1 * lightColor1;
    vec3 diffuse2 = diff2 * lightColor2;

    vec3 ambient1 = ambientStrength * lightColor1;
    vec3 ambient2 = ambientStrength * lightColor2;

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir1 = reflect(-lightDir1, norm);  
    vec3 reflectDir2 = reflect(-lightDir2, norm);  
    
    float spec1 = pow(max(dot(viewDir, reflectDir1), 0.0), shininess);
    float spec2 = pow(max(dot(viewDir, reflectDir2), 0.0), shininess);
    vec3 specular1 = specularStrength * spec1 * lightColor1;  
    vec3 specular2 = specularStrength * spec2 * lightColor2;  
    
    vec3 resultingColour = objectColor*(ambient1+ambient2+diffuse1+diffuse2 + specular1+specular2);
    outColor = vec4(resultingColour,1);

   
}
