#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include <iostream>

constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 720;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

int main(void)
{
    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Graphics 1", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    /* Load OpenGL 4.6 (Windows ships with GL 1.0, we don't like GL 1.0)*/
    gladLoadGL();

    glfwSetKeyCallback(window, key_callback);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        /* Render here */
        glClearColor(0.22f, 0.49f, 0.17f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    const char* name = glfwGetKeyName(key, scancode);
    if (action == GLFW_PRESS)
        printf("%s\n", name);
    //if (key == GLFW_KEY_E && action == GLFW_PRESS)
    //    activate_airship();
}
