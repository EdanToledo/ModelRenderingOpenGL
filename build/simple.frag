#version 330 core

uniform vec3 objectcolour;
uniform vec3 lightSource1;
uniform vec3 lightcolour1;
uniform vec3 lightSource2;
uniform vec3 lightcolour2;
uniform vec3 viewPos;
uniform sampler2D textureImage;
uniform sampler2D normalMap;

in vec3 FragPos;  
in vec3 normal;
in vec2 TexCoord;
in mat3 TBN;

out vec4 outcolour;

float ambientStrength = 0.1;

float specularStrength = 0.8;
float shininess = 32;

void main()
{


   vec3 normal = texture(normalMap, TexCoord).rgb;

   normal = normal * 2.0 - 1.0;   
   normal = (TBN * normal); 

   vec3 norm = normalize(normal);
  
   vec3 lightDir1 = normalize(lightSource1 - FragPos);  
   vec3 lightDir2 = normalize(lightSource2 - FragPos);  

    vec3 texcolour =texture(textureImage,TexCoord).rgb;
    
    float diff1 = max(dot(lightDir1,norm), 0.0);
    float diff2 = max(dot(lightDir2,norm), 0.0);
    
    vec3 diffuse1 = diff1 * lightcolour1*texcolour;
    vec3 diffuse2 = diff2 * lightcolour2*texcolour;

    vec3 ambient1 = ambientStrength * lightcolour1*texcolour;
    vec3 ambient2 = ambientStrength * lightcolour2*texcolour;

   vec3 viewDir = normalize(viewPos - FragPos);
   
    vec3 reflectDir1 = reflect(-lightDir1, norm);  
    vec3 reflectDir2 = reflect(-lightDir2, norm);  
    
    float spec1 = pow(max(dot(viewDir, reflectDir1), 0.0), shininess);
    float spec2 = pow(max(dot(viewDir, reflectDir2), 0.0), shininess);
    vec3 specular1 = specularStrength * spec1 * lightcolour1;  
    vec3 specular2 = specularStrength * spec2 * lightcolour2;  
    
    vec3 resultingColour = (ambient1+ambient2+diffuse1+diffuse2 + specular1+specular2);
    outcolour = vec4(resultingColour,1);

   
}
