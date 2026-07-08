#include <iostream>
#include <glm/glm.hpp>
#include <glad/glad.h>

struct GPU_Object{
    GLuint vao, vbo, ebo;
    uint32_t index_count;
    GPU_Object(){
        glGenBuffers(1,&vbo);
        glGenBuffers(1,&ebo);
        glGenVertexArrays(1,&vao);
    }
    ~GPU_Object(){
        glDeleteBuffers(1,&vbo);
        glDeleteBuffers(1,&ebo);
        glDeleteVertexArrays(1,&vao);
    }
};
