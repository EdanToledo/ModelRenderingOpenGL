#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexture;

out vec3 normal;
out vec3 FragPos;
out vec2 TexCoord;

uniform mat4 Model[2];
uniform mat4 View[2];
uniform mat4 Projection[2];

void main()
{
    gl_Position = Projection[gl_InstanceID] *View[gl_InstanceID]*Model[gl_InstanceID] * vec4(position,1.0f);
    normal = mat3(transpose(inverse(Model[gl_InstanceID])))*aNormal;
    FragPos = vec3(Model[gl_InstanceID] * vec4(position, 1.0));
    TexCoord = aTexture;
}
