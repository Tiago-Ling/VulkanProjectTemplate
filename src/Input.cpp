#include "Input.hpp"

namespace {
    template <size_t N>
    bool get(const std::array<bool, N>& states, int code) {
        return code >= 0 && code < static_cast<int>(N) && states[code];
    }
}

// Initialize input system
void Input::init(GLFWwindow* win) {
    window = win;

    // Sticky input keeps a press visible until the next poll, so a key tapped between two frames is not missed
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);
    glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);
    glfwSetScrollCallback(window, scrollCallback);

    glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
}

// Call this once per frame
void Input::pollEvents() {
    previousKeys = keys;
    previousButtons = buttons;
    scrollX = 0.0;
    scrollY = 0.0;

    glfwPollEvents(); // runs scrollCallback for any scroll events since the last frame

    // Update key and button states
    for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; ++key) {
        keys[key] = (glfwGetKey(window, key) == GLFW_PRESS);
    }
    for (int button = 0; button <= GLFW_MOUSE_BUTTON_LAST; ++button) {
        buttons[button] = (glfwGetMouseButton(window, button) == GLFW_PRESS);
    }

    // Update mouse position delta
    updateMouse();
}

// Track relative mouse movement
void Input::updateMouse() {
    double currentX, currentY;
    glfwGetCursorPos(window, &currentX, &currentY);

    deltaX = currentX - lastMouseX;
    deltaY = currentY - lastMouseY;

    lastMouseX = currentX;
    lastMouseY = currentY;
}

// Accumulate scroll offsets; several events can arrive in one frame
void Input::scrollCallback(GLFWwindow* /*window*/, double offsetX, double offsetY) {
    scrollX += offsetX;
    scrollY += offsetY;
}

bool Input::isKeyDown(int key) {
    return get(keys, key);
}

bool Input::wasKeyPressed(int key) {
    return get(keys, key) && !get(previousKeys, key);
}

bool Input::wasKeyReleased(int key) {
    return !get(keys, key) && get(previousKeys, key);
}

bool Input::isMouseButtonDown(int button) {
    return get(buttons, button);
}

bool Input::wasMouseButtonPressed(int button) {
    return get(buttons, button) && !get(previousButtons, button);
}

bool Input::wasMouseButtonReleased(int button) {
    return !get(buttons, button) && get(previousButtons, button);
}

// Get mouse movement since last frame
double Input::getMouseDeltaX() {
    return deltaX;
}

double Input::getMouseDeltaY() {
    return deltaY;
}

// Get scroll wheel offset since last frame (positive Y scrolls up)
double Input::getScrollDeltaX() {
    return scrollX;
}

double Input::getScrollDeltaY() {
    return scrollY;
}

void Input::setCursorCaptured(bool captured) {
    cursorCaptured = captured;
    glfwSetInputMode(window, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

    // Raw motion skips OS pointer acceleration; only available (and only useful) while captured
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, captured ? GLFW_TRUE : GLFW_FALSE);
    }

    // The cursor position jumps when the mode changes; start the next delta from the new position
    glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
}

bool Input::isCursorCaptured() {
    return cursorCaptured;
}
