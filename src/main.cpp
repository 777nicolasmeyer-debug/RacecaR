#include "../Engine/Engine.h"

#include <cstdio>
#include <iostream>

#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace {
    struct ApplicationState {
        Engine *engine = nullptr;
        bool spawnLightWasDown = false;
        bool resetWasDown = false;
        bool playerWasDown = false;
    };

    void mouseCallback(GLFWwindow *window, const double xpos, const double ypos) {
        auto *state = static_cast<ApplicationState *>(glfwGetWindowUserPointer(window));
        if (state != nullptr && state->engine != nullptr) {
            state->engine->handleMouseInput(xpos, ypos);
        }
    }

    Engine::CameraInput readCameraInput(GLFWwindow *window) {
        Engine::CameraInput input;
        input.forward = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
        input.backward = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
        input.left = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
        input.right = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
        input.up = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        input.down = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
        input.brake = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        input.shiftUp = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
        input.shiftDown = glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS;
        return input;
    }
}

int main() {
    if (glfwInit() != GLFW_TRUE) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(800, 600, "RacecaR", nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    int result = 0;
    {
        Engine engine;
        ApplicationState app{&engine};
        glfwSetWindowUserPointer(window, &app);
        glfwSetCursorPosCallback(window, mouseCallback);

        if (!engine.initialize()) {
            result = 1;
        } else {                                //track one is degenerate, it wont work
            engine.addStaticModel("../assets/F1_track2.glb", "../assets/track2_collision.glb", glm::vec3(0.0f));
            engine.addLight("../assets/Light.glb", glm::vec3(900.0f, 1500.0f, 650.0f), glm::vec3(1.0f), false, true);
            glClearColor(0.4f, 0.6f, 0.8f, 1.0f);
            float lastFrame = static_cast<float>(glfwGetTime());
            while (!glfwWindowShouldClose(window)) {
                const float currentFrame = static_cast<float>(glfwGetTime());
                const float deltaTime = currentFrame - lastFrame;
                lastFrame = currentFrame;

                const bool playerDown = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;
                if (playerDown && !app.playerWasDown) {
                    engine.togglePlayer();
                }
                app.playerWasDown = playerDown;

                engine.handleKeyboardInput(readCameraInput(window), deltaTime);

                const bool lightDown = glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS;
                if (lightDown && !app.spawnLightWasDown) {
                    engine.addLight("../assets/Light.glb", engine.getCameraPosition());
                }
                app.spawnLightWasDown = lightDown;

                const bool resetDown = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
                if (resetDown && !app.resetWasDown) {
                    engine.clearDynamicObjects();
                    engine.clearLights();
                }
                app.resetWasDown = resetDown;

                if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
                }

                engine.update(deltaTime);
                if (engine.hasPlayer()) {
                    char title[80];
                    std::snprintf(title, sizeof(title), "RacecaR | Gear %d | %.0f RPM",
                                  engine.getPlayerGear(), engine.getPlayerRPM());
                    glfwSetWindowTitle(window, title);
                } else {
                    glfwSetWindowTitle(window, "RacecaR");
                }
                int width = 0;
                int height = 0;
                glfwGetFramebufferSize(window, &width, &height);
                engine.render(width, height);

                glfwSwapBuffers(window);
                glfwPollEvents();
            }
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return result;
}
