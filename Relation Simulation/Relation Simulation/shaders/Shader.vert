#version 430 core

struct Person{
    uint id;
    uint rel_offset;
    uint rel_count;
    vec2 position;
    vec2 velocity;
    vec2 acceleration;
    vec4 color;
};

layout(location = 0) in vec2 position;
layout(std430, binding = 3) buffer Position{
    Person pos[];
};

uniform mat4 proj;
uniform mat4 view;

out vec4 color;

void main(){
    gl_Position = proj * view * vec4((pos[gl_InstanceID].position+position)*0.06f,0.f,1.f);
    color = pos[gl_InstanceID].color;
}
