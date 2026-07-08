#include "camera.h"

void Camera::Camera_move(glm::vec2 move_direction){
    this->position += move_direction*speed;
};
glm::mat4 Camera::get_camera_update(){
    glm::vec3 eye = glm::vec3(this->position, -1.f);
    return glm::lookAt(eye,eye+this->direction,glm::vec3(0.f,1.f,0.f));;
};
