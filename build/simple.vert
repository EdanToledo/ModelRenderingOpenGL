#version 330 core

in vec3 position;

uniform mat4 MVP[2];

void main()
{
    gl_Position = MVP[gl_InstanceID] * vec4(position,1.0f);
}
