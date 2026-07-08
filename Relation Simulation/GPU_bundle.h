#ifndef GPU_BUNDLE_H
#define GPU_BUNDLE_H
#include "GPU_Object.cpp"
#include "mesh.cpp"


struct Shader{
    GLuint shader_id;
    GLenum shader_flag;
    bool get_shader(std::string filepath);
    Shader(GLenum type);
    void destroy();
    ~Shader();
};

struct Shader_program{
    GLint program_id;

    void Compile_program();

    void Add_Shader(Shader &shader);

    Shader_program();
};

Mesh create_circle_mesh(float radius, uint8_t steps);

void create_object_from_circular_mesh(Mesh &mesh, GPU_Object *target);


#endif // GPU_BUNDLE_H
