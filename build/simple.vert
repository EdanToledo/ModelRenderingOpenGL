#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexture;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec3 inBitangent;  

out vec3 normal;
out vec3 FragPos;
out vec2 TexCoord;
out mat3 TBN;

uniform mat4 Model[2];
uniform mat4 View[2];
uniform mat4 Projection[2];

void main()
{
    gl_Position = Projection[gl_InstanceID] *View[gl_InstanceID]*Model[gl_InstanceID] * vec4(position,1.0f);
    normal = mat3(transpose(inverse(Model[gl_InstanceID])))*inNormal;
    FragPos = vec3(Model[gl_InstanceID] * vec4(position, 1.0));
    TexCoord = inTexture;
    
    vec3 T = normalize(vec3(transpose(inverse(Model[gl_InstanceID])) * vec4(inTangent,   0.0)));
   vec3 B = normalize(vec3(transpose(inverse(Model[gl_InstanceID])) * vec4(inBitangent, 0.0)));
   vec3 N = normalize(vec3(transpose(inverse(Model[gl_InstanceID]))* vec4(inNormal,    0.0)));
    TBN = (mat3(T, B, N));
  
}
