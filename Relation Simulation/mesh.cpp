#ifndef MESH
#define MESH


#include "glad/glad.h"
#include <vector>
#include <glm/glm.hpp>
#include "Constants.h"

struct Mesh{
    std::vector<glm::vec2> vertices; //always assumes a triangle of vertices[n], vertices[n+1], vertices[n+2] and looping back at 0, unless shown otherwise
    ~Mesh(){
        vertices.~vector();
    };
};

#endif // MESH
