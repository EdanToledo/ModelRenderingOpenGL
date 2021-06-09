#version 330 core

uniform vec3 objectColor;
uniform vec3 lightSource;
uniform vec3 lightColor;

in vec3 FragPos;  
in vec3 normal;

out vec4 outColor;

void main()
{
    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(lightSource - FragPos);  

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;

    vec3 resultingColour = objectColor*(ambient+diffuse);
    outColor = vec4(resultingColour,1);

   
   
}
