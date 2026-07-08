#version 330 core

layout (location=0) in vec2 position;

uniform mat4 proj;
uniform mat4 view;

out vec4 color;

void main(){
    gl_Position = proj * view * vec4(position*0.06f,0.f,1.f);
    color = vec4(1.f,1.f,1.f,1.f);
}
