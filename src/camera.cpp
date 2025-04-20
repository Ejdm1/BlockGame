#include "camera.hpp"

//////////////////////////////////////Calculate camera rotation///////////////////////////////////////////
void Camera::on_mouse_move(GLFWwindow* glfwWindow, glm::vec2 cursor_pos, glm::vec2 window_size) {

    /////////////Calculate the vursor move and rotation/////////////////
    if(glfwGetWindowAttrib(glfwWindow, GLFW_FOCUSED)) {
        cursor_pos.x = window_size.x/2 - cursor_pos.x;
        cursor_pos.y = window_size.y/2 - cursor_pos.y;
        rotation.x += cursor_pos.x * mouse_sens * 0.0001f * fov;
        rotation.y += cursor_pos.y * mouse_sens * 0.0001f * fov;
    }
    ////////////////////////////////////////////////////////////////////

    ///Set maximum rotation so you cant move more than set value////
    const float MAX_ROT = 1.5f;
    if (rotation.y > MAX_ROT) {rotation.y = MAX_ROT;}
    if (rotation.y < -MAX_ROT) {rotation.y = -MAX_ROT;}
    ///////////////////////////////////////////////////////////////
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////Update camera and react to user inputs (keyboard events)//////////////////////////////////////////////////
void Camera::update(MTL::Buffer* CameraDataBuffer, GLFWwindow* glfwWindow, glm::vec2 window_size , float delta_time, Window* window) {
    CameraData = reinterpret_cast<CameraDataS*>(CameraDataBuffer->contents());
    CameraData->perspectiveTransform = math::makePerspective(fov * M_PI / 180.f, window->aspect_ratio, 0.03f, 1000.0f);

    ///////////////////////////////////////Set cursor state and process mouse movement//////////////////////////////////
    if(!window->menu) {
        glfwGetCursorPos(window->glfwWindow, &x, &y);
        mouse_pos_old = mouse_pos_new; mouse_pos_new.x = static_cast<float>(x); mouse_pos_new.y = static_cast<float>(y);

        if (mouse_pos_old != mouse_pos_new && glfwGetWindowAttrib(glfwWindow, GLFW_FOCUSED)) {
            on_mouse_move(window->glfwWindow, mouse_pos_new, window->window_size);
        }

        if (glfwGetWindowAttrib(glfwWindow, GLFW_FOCUSED)) {
            window->cursor_setup(true, glfwWindow, window_size);
        }
        else if (!glfwGetWindowAttrib(glfwWindow, GLFW_FOCUSED)) {
            window->cursor_setup(false, glfwWindow, window_size);
        }
    }
    else if(window->menu) {
        window->cursor_setup(false, glfwWindow, window_size);
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////Calculating directions and reseting move direction/////////////////////////////////////////////////////////////
    glm::vec3 forward_direction = glm::normalize(glm::vec3{glm::cos(rotation.x) * glm::cos(rotation.y), glm::sin(rotation.y), -glm::sin(rotation.x) * glm::cos(rotation.y)});
    glm::vec3 up_direction = glm::vec3{0.0f, 1.0f, 0.0f};
    glm::vec3 right_direction = glm::normalize(glm::cross(forward_direction, up_direction));

    glm::vec3 move_direction = glm::vec3{0.0f};
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////Process keyboard events/////////////////////////////////////////////////////////////////
    if(glfwGetWindowAttrib(glfwWindow, GLFW_FOCUSED)) { 
        if (glfwGetKey(glfwWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            esc_pressed = true;
        }
        if(!window->menu) {
            if (glfwGetKey(glfwWindow, GLFW_KEY_W) == GLFW_PRESS) {move_direction += forward_direction;}
            if (glfwGetKey(glfwWindow, GLFW_KEY_S) == GLFW_PRESS) {move_direction -= forward_direction;}
            if (glfwGetKey(glfwWindow, GLFW_KEY_D) == GLFW_PRESS) {move_direction += right_direction;}
            if (glfwGetKey(glfwWindow, GLFW_KEY_A) == GLFW_PRESS) {move_direction -= right_direction;}
            if (glfwGetKey(glfwWindow, GLFW_KEY_SPACE) == GLFW_PRESS) {move_direction += up_direction;}
            if (glfwGetKey(glfwWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {move_direction -= up_direction;}
            if (glfwGetKey(glfwWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {position += move_direction * delta_time * 50.f;}
            else if (glfwGetKey(glfwWindow, GLFW_KEY_LEFT_SHIFT) != GLFW_PRESS) {position += move_direction * delta_time * 7.5f;}
        }
        ////////////////////////////////////////////////Set cursor state//////////////////////////////////////////////////
        if (glfwGetKey(glfwWindow, GLFW_KEY_ESCAPE) == GLFW_RELEASE && esc_pressed) {
            if(!window->menu) {
                window->menu = true;
                window->cursor_setup(false, glfwWindow, window_size);
            }
            else {
                window->menu = false; 
                window->cursor_setup(true, glfwWindow, window_size);
            }
            esc_pressed = false; 
        }
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////Update view matrix//////
    view_mat = glm::lookAt(position, position + forward_direction, up_direction);
    ///////////////////////////////
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////