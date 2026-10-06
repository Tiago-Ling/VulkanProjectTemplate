#pragma once

#include <GLFW/glfw3.h>
#include <array>

// Keyboard and mouse state, updated once per frame by pollEvents().
// "Down" is the current state; "Pressed" and "Released" are true only on the frame the state changed.
class Input {
public:
    static void init(GLFWwindow* window);

    static void pollEvents(); // Must be called every frame

    // Keys use GLFW_KEY_* codes; unknown codes report false
    static bool isKeyDown(int key);
    static bool wasKeyPressed(int key);
    static bool wasKeyReleased(int key);

    // Buttons use GLFW_MOUSE_BUTTON_* codes; unknown codes report false
    static bool isMouseButtonDown(int button);
    static bool wasMouseButtonPressed(int button);
    static bool wasMouseButtonReleased(int button);

    // Cursor movement and scroll wheel offset since the previous frame
    static double getMouseDeltaX();
    static double getMouseDeltaY();
    static double getScrollDeltaX();
    static double getScrollDeltaY();

    // A captured cursor is hidden and locked to the window, with unbounded movement (e.g. for mouse-look cameras)
    static void setCursorCaptured(bool captured);
    static bool isCursorCaptured();

private:
    static inline GLFWwindow* window = nullptr;

    // Indexed by GLFW key and mouse button codes
    static inline std::array<bool, GLFW_KEY_LAST + 1> keys{};
    static inline std::array<bool, GLFW_KEY_LAST + 1> previousKeys{};
    static inline std::array<bool, GLFW_MOUSE_BUTTON_LAST + 1> buttons{};
    static inline std::array<bool, GLFW_MOUSE_BUTTON_LAST + 1> previousButtons{};

    static inline double lastMouseX = 0.0;
    static inline double lastMouseY = 0.0;
    static inline double deltaX = 0.0;
    static inline double deltaY = 0.0;
    static inline double scrollX = 0.0;
    static inline double scrollY = 0.0;
    static inline bool cursorCaptured = false;

    static void updateMouse();
    static void scrollCallback(GLFWwindow* window, double offsetX, double offsetY);
};
