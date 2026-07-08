#ifndef SIMPLE_CAMERA_2D_H
#define SIMPLE_CAMERA_2D_H
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Camera{
    float speed; //4 bytes
    glm::vec3 direction; //12 bytes
    glm::vec2 position; //8 bytes
    float zoom; //4 bytes
    //no memory padding hopefully!

    //!Note that library expects the coder to multiply a normalized direction by delta time
    void Camera_move(glm::vec2 move_direction);
    glm::mat4 get_camera_update();
};

#endif // _SIMPLE_CAMERA_2D_H
