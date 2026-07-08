#include "GPU_bundle.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

Mesh create_circle_mesh(float radius, uint8_t steps){
    Mesh result;
    float step_size = 2.0f*pi/steps;
    result.vertices.push_back(glm::vec2(0.f,0.f));
    for(int i = 0; i < steps; i++){
        result.vertices.push_back(radius*glm::vec2(cos(i*step_size),sin(i*step_size)));
    }
    return result;
}

void create_object_from_circular_mesh(Mesh &mesh, GPU_Object *target){

    glBindVertexArray(target->vao);
    glBindBuffer(GL_ARRAY_BUFFER,target->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,target->ebo);

    glBufferData(GL_ARRAY_BUFFER,sizeof(glm::vec2)*mesh.vertices.size(),mesh.vertices.data(),GL_DYNAMIC_DRAW);

    std::vector<GLuint> index_buffer;
    for(int i = 1;i<mesh.vertices.size()-1;i++){
        index_buffer.push_back(0);
        index_buffer.push_back(i);
        index_buffer.push_back(i+1);
    }
    index_buffer.push_back(0);
    index_buffer.push_back(mesh.vertices.size()-1);
    index_buffer.push_back(1);
    target->index_count = 3*mesh.vertices.size();

    glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(GLuint)*index_buffer.size(),index_buffer.data(),GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,2*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
    glBindBuffer(GL_ARRAY_BUFFER,0);
}

bool Shader::get_shader(std::string filepath){
    std::ifstream shader_file(filepath);
    std::stringstream shader_source;
    std::string shader_string;
    shader_source<<shader_file.rdbuf();
    shader_string = shader_source.str();
    const GLchar* shader_code = shader_string.c_str();

    glShaderSource(shader_id, 1, &shader_code, NULL);
    glCompileShader(shader_id);

    GLint success;
    GLchar infoLog[1024];

    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);

    if(!success){
        std::cout << shader_flag << "\n";
        glGetShaderInfoLog(shader_id, 1024, NULL, infoLog);
        std::cout << "Shader compile error:\n" << infoLog << "\n";
        return false;
    }

    return true;
}

Shader::Shader(GLenum type){
    shader_id = glCreateShader(type);
    shader_flag = type;
}

Shader::~Shader(){
    glDeleteShader(shader_id);
}

void Shader::destroy(){
    glDeleteShader(shader_id);
}

Shader_program::Shader_program(){
    program_id = glCreateProgram();
}

void Shader_program::Add_Shader(Shader &shader){
    glAttachShader(program_id,shader.shader_id);

}
