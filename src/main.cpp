#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cstdlib>
#include <iostream>

constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 720;

float Random(float min, float max)
{
    return min + (rand() / ((float)RAND_MAX / (max - min)));
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

int main(void)
{
    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Graphics 1", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -2;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    /* Load OpenGL 4.6 (Windows ships with GL 1.0, we don't like GL 1.0)*/
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        glfwTerminate();
        return -3;
    }

    glfwSetKeyCallback(window, key_callback);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        /* Render here */
        glClearColor(0.22f, 0.49f, 0.17f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        float x = Random(0.0f, SCREEN_WIDTH);
        float y = Random(0.0f, SCREEN_HEIGHT);
        //glfwSetCursorPos(window, x, y);

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

    if (key == GLFW_KEY_ESCAPE)
        glfwSetWindowShouldClose(window, GLFW_TRUE);

    // Homework: find a way to consume key inputs to make this an even better virus!
}
