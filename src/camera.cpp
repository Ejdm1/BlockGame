#include "math.cpp"

#include "camera.hpp"

void Camera::on_mouse_move(GLFWwindow* glfwWindow, double x, double y, glm::vec2 window_size) {
    x = window_size.x/2 - x;
    y = window_size.y/2 - y;
    if(glfwGetWindowAttrib(glfwWindow, GLFW_FOCUSED)) {
        rotation.x += static_cast<float>(x) * mouse_sens * 0.0001f * fov;
        rotation.y += static_cast<float>(y) * mouse_sens * 0.0001f * fov;
    }

    constexpr auto MAX_ROT = 1.56825555556f;
    if (rotation.y > MAX_ROT) { rotation.y = MAX_ROT; }
    if (rotation.y < -MAX_ROT) { rotation.y = -MAX_ROT; }
}

void Camera::update(MTL::Buffer* pCameraDataBuffer, GLFWwindow* glfwWindow, glm::vec2 window_size , float delta_time, Window* window) {
    pCameraData = reinterpret_cast< CameraData *>( pCameraDataBuffer->contents() );
    pCameraData->perspectiveTransform = math::makePerspective( fov * M_PI / 180.f, 1.f, 0.03f, 1000.0f ) ;

    if (glfwGetWindowAttrib(glfwWindow, GLFW_FOCUSED)) {
        window->cursor_setup(true, glfwWindow, window_size);
    }
    else if (!glfwGetWindowAttrib(glfwWindow, GLFW_FOCUSED)) {
        window->cursor_setup(false, glfwWindow, window_size);
    }

    glm::vec3 forward_direction = glm::normalize(glm::vec3{ glm::cos(rotation.x) * glm::cos(rotation.y), glm::sin(rotation.y), -glm::sin(rotation.x) * glm::cos(rotation.y) });
    glm::vec3 up_direction = glm::vec3{ 0.0f, 1.0f, 0.0f };
    glm::vec3 right_direction = glm::normalize(glm::cross(forward_direction, up_direction));

    glm::vec3 move_direction = glm::vec3{ 0.0f };

    if(glfwGetWindowAttrib(glfwWindow, GLFW_FOCUSED)) { 
        if (glfwGetKey(glfwWindow, GLFW_KEY_W)  == GLFW_PRESS) { move_direction += forward_direction; }
        if (glfwGetKey(glfwWindow, GLFW_KEY_S)  == GLFW_PRESS) { move_direction -= forward_direction; }
        if (glfwGetKey(glfwWindow, GLFW_KEY_D)  == GLFW_PRESS) { move_direction += right_direction; }
        if (glfwGetKey(glfwWindow, GLFW_KEY_A)  == GLFW_PRESS) { move_direction -= right_direction; }
        if (glfwGetKey(glfwWindow, GLFW_KEY_SPACE)  == GLFW_PRESS) { move_direction += up_direction; }
        if (glfwGetKey(glfwWindow, GLFW_KEY_LEFT_CONTROL)  == GLFW_PRESS) { move_direction -= up_direction; }
        if (glfwGetKey(glfwWindow, GLFW_KEY_LEFT_SHIFT)  == GLFW_PRESS) { position += move_direction * delta_time * 15.f; }
        else if (glfwGetKey(glfwWindow, GLFW_KEY_LEFT_SHIFT)  != GLFW_PRESS) { position += move_direction * delta_time * 7.5f; }
        if (glfwGetKey(glfwWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS) {glfwSetWindowShouldClose(glfwWindow, GLFW_TRUE); }
    }

    view_mat = glm::lookAt(position, position + forward_direction, up_direction);
}