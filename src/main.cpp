#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Math.h"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <array>

constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 720;

void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char* message, const void* userParam);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

GLuint CreateShader(GLint type, const char* path);
GLuint CreateProgram(GLuint vs, GLuint fs);

std::array<int, GLFW_KEY_LAST> gKeysCurr{}, gKeysPrev{};
bool IsKeyDown(int key);
bool IsKeyUp(int key);
bool IsKeyPressed(int key);

void Print(Matrix m);

int main(void)
{
    // Lines 20-40 are all window creation. You can ignore this if you want ;)
    assert(glfwInit() == GLFW_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
#ifdef NDEBUG
#else
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Graphics 1", NULL, NULL);
    glfwMakeContextCurrent(window);
    assert(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress));
    glfwSetKeyCallback(window, key_callback);

#ifdef NDEBUG
#else
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(glDebugOutput, nullptr);
#endif

    // Vertex shaders:
    GLuint vs = CreateShader(GL_VERTEX_SHADER, "./assets/shaders/default.vert");
    GLuint vsLines = CreateShader(GL_VERTEX_SHADER, "./assets/shaders/lines.vert");
    GLuint vsVertexPositionColor = CreateShader(GL_VERTEX_SHADER, "./assets/shaders/vertex_color.vert");
    GLuint vsColorBufferColor = CreateShader(GL_VERTEX_SHADER, "./assets/shaders/buffer_color.vert");
    
    // Fragment shaders:
    GLuint fsLines = CreateShader(GL_FRAGMENT_SHADER, "./assets/shaders/lines.frag");
    GLuint fsUniformColor = CreateShader(GL_FRAGMENT_SHADER, "./assets/shaders/uniform_color.frag");
    GLuint fsVertexColor = CreateShader(GL_FRAGMENT_SHADER, "./assets/shaders/vertex_color.frag");
    
    // Shader programs:
    GLuint shaderUniformColor = CreateProgram(vs, fsUniformColor);
    GLuint shaderVertexPositionColor = CreateProgram(vsVertexPositionColor, fsVertexColor);
    GLuint shaderVertexBufferColor = CreateProgram(vsColorBufferColor, fsVertexColor);
    GLuint shaderLines = CreateProgram(vsLines, fsLines);

    // Positions of our triangle's vertices (CCW winding-order)
    Vector3 positions[] =
    {
        0.5f, -0.5f, 0.0f,  // vertex 1 (bottom-right)
        0.0f, 0.5f, 0.0f,   // vertex 2 (top-middle)
        -0.5f, -0.5f, 0.0f  // vertex 3 (bottom-left)
    };

    // Colours of our triangle's vertices (xyz = rgb)
    Vector3 colours[] =
    {
        1.0f, 0.0f, 0.0f,   // vertex 1
        0.0f, 1.0f, 0.0f,   // vertex 2
        0.0f, 0.0f, 1.0f    // vertex 3
    };

    Vector2 curr[4]
    {
        { -1.0f,  1.0f },   // top-left
        {  1.0f,  1.0f },   // top-right
        {  1.0f, -1.0f },   // bot-right
        { -1.0f, -1.0f }    // bot-left
    };

    Vector2 next[4]
    {
        (curr[0] + curr[1]) * 0.5f,
        (curr[1] + curr[2]) * 0.5f,
        (curr[2] + curr[3]) * 0.5f,
        (curr[3] + curr[0]) * 0.5f
    };

    // vao = "Vertex Array Object". A vao is a collection of vbos.
    // vbo = "Vertex Buffer Object". "Buffer" generally means "group of memory".
    // A vbo is a piece of graphics memory VRAM.
    GLuint vao, pbo, cbo;       // pbo = "position buffer object", "cbo = color buffer object"
    glGenVertexArrays(1, &vao); // Allocate a vao handle
    glBindVertexArray(vao);     // Bind = "associate all bound buffer object with the current array object"
    
    // Create position buffer:
    glGenBuffers(1, &pbo);              // Allocate a vbo handle
    glBindBuffer(GL_ARRAY_BUFFER, pbo); // Associate this buffer with the bound vertex array
    glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(Vector3), positions, GL_STATIC_DRAW);  // Upload the buffer
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), 0);          // Describe the buffer
    glEnableVertexAttribArray(0);

    // Create color buffer:
    glGenBuffers(1, &cbo);              // Allocate a vbo handle
    glBindBuffer(GL_ARRAY_BUFFER, cbo); // Associate this buffer with the bound vertex array
    glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(Vector3), colours, GL_STATIC_DRAW);    // Upload the buffer
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), 0);          // Describe the buffer
    glEnableVertexAttribArray(1);

    GLuint vaoLines, pboLines, cboLines;
    glGenVertexArrays(1, &vaoLines);
    glBindVertexArray(vaoLines);

    glGenBuffers(1, &pboLines);
    glBindBuffer(GL_ARRAY_BUFFER, pboLines);
    glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(Vector2), curr, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vector2), nullptr);
    glEnableVertexAttribArray(0);

    // In summary, we need 3 things to render:
    // 1. Vertex data -- right now just positions.
    // 2. Shader -- vs forwards input, fs colours.
    // 3. Draw call -- draw 3 vertices interpreted as a triangle (GL_TRIANGLES)
    // *** Everything is just data and behaviour ***
    // *** vao & vbo describe data, shaders describe behaviour ***

    // Fetch handles to uniform ("constant") variables.
    // OpenGL handles are like addresses (&) in c++ -- they tell us the location of our data on the GPU.
    // In the case of uniforms, we need to know their handle (location) before we can use them!
    GLint u_color = glGetUniformLocation(shaderUniformColor, "u_color");
    GLint u_intensity = glGetUniformLocation(shaderUniformColor, "u_intensity");

    int object = 0;
    printf("Object %i\n", object + 1);

    Matrix view = LookAt({ 0.0f, 0.0f, 5.0f }, V3_ZERO, V3_UP);
    Matrix proj = Ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 10.0f);
    // Optional homework: use the Perspective function to see how the projection changes!

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        float time = glfwGetTime();

        // Interpolation parameter (0 means fully A, 1 means fully B)
        float a = cosf(time) * 0.5f + 0.5f;

        // Interpolate scale
        Vector3 sA = V3_ONE;
        Vector3 sB = V3_ONE * 10.0f;
        Vector3 sC = Lerp(sA, sB, a);

        // Interpolate rotation (slerp = "spherical lerp" because we rotate in a circle) 
        Quaternion rA = QuaternionIdentity();
        Quaternion rB = FromEuler(0.0f, 0.0f, 90.0f * DEG2RAD);
        Quaternion rC = Slerp(rA, rB, a);

        // Interpolate translation
        Vector3 tA = { -10.0f, 0.0f, 0.0f };
        Vector3 tB = {  10.0f, 0.0f, 0.0f };
        Vector3 tC = Lerp(tA, tB, a);

        // Interpolate color
        Vector3 cA = V3_UP;
        Vector3 cB = V3_FORWARD;
        Vector3 cC = Lerp(cA, cB, a);

        Matrix s = Scale(sC);
        Matrix r = ToMatrix(rC);
        Matrix t = Translate(tC);

        Matrix world = s * r * t;
        Matrix mvp = world * view * proj;
        GLint u_mvp = GL_NONE;

        GLuint shaderProgram = GL_NONE;

        switch (object + 1)
        {
        case 1:
            shaderProgram = shaderVertexBufferColor;
            glUseProgram(shaderProgram);
            u_mvp = glGetUniformLocation(shaderProgram, "u_mvp");
            glUniformMatrix4fv(u_mvp, 1, GL_FALSE, ToFloat16(mvp).v);
            //glUniform3fv(u_color, 1, &cC.x);
            //glUniform1f(u_intensity, a);
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            break;

        case 2:
            shaderProgram = shaderVertexBufferColor;
            glUseProgram(shaderProgram);
            u_mvp = glGetUniformLocation(shaderProgram, "u_mvp");
            glUniformMatrix4fv(u_mvp, 1, GL_FALSE, ToFloat16(mvp).v);
            //glUniform3fv(u_color, 1, &cC.x);
            //glUniform1f(u_intensity, 1.0f);
            glBindVertexArray(vao);
            glDrawArrays(GL_LINE_LOOP, 0, 3);
            break;

        case 3:
            // TODO -- read up on glBufferSubData to understand what on earth just happened ;)
            shaderProgram = shaderLines;
            glUseProgram(shaderProgram);
            glUniform1f(glGetUniformLocation(shaderProgram, "u_a"), a);
            glLineWidth(10.0f);
            glBindVertexArray(vaoLines);
            glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(Vector2), curr);
            glDrawArrays(GL_LINE_LOOP, 0, 4);
            glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(Vector2), next);
            glDrawArrays(GL_LINE_LOOP, 0, 4);
            break;

        case 4:
            shaderProgram = shaderUniformColor;
            glUseProgram(shaderProgram);
            u_mvp = glGetUniformLocation(shaderProgram, "u_mvp");
            glUniformMatrix4fv(u_mvp, 1, GL_FALSE, ToFloat16(mvp).v);
            glUniform3fv(u_color, 1, &cC.x);
            glUniform1f(u_intensity, 1.0);
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            break;

        case 5:
            shaderProgram = shaderUniformColor;
            glUseProgram(shaderProgram);
            u_mvp = glGetUniformLocation(shaderProgram, "u_mvp");
            glUniformMatrix4fv(u_mvp, 1, GL_FALSE, ToFloat16(mvp).v);
            glUniform3fv(u_color, 1, &cC.x);
            glUniform1f(u_intensity, 1.0f - a);
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            break;
        }

        // Change object when space is pressed
        if (IsKeyPressed(GLFW_KEY_SPACE))
        {
            ++object %= 5;
            printf("Object %i\n", object + 1);
        }

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll and process events */
        memcpy(gKeysPrev.data(), gKeysCurr.data(), GLFW_KEY_LAST * sizeof(int));
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Disable repeat events so keys are either up or down
    if (action == GLFW_REPEAT) return;

    if (key == GLFW_KEY_ESCAPE)
        glfwSetWindowShouldClose(window, GLFW_TRUE);

    gKeysCurr[key] = action;

    // Key logging
    //const char* name = glfwGetKeyName(key, scancode);
    //if (action == GLFW_PRESS)
    //    printf("%s\n", name);
}

// Compile a shader
GLuint CreateShader(GLint type, const char* path)
{
    GLuint shader = GL_NONE;
    try
    {
        // Load text file
        std::ifstream file;
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        file.open(path);

        // Interpret the file as a giant string
        std::stringstream stream;
        stream << file.rdbuf();
        file.close();

        // Verify shader type matches shader file extension
        const char* ext = strrchr(path, '.');
        switch (type)
        {
        case GL_VERTEX_SHADER:
            assert(strcmp(ext, ".vert") == 0);
            break;

        case GL_FRAGMENT_SHADER:
            assert(strcmp(ext, ".frag") == 0);
            break;
        default:
            assert(false, "Invalid shader type");
            break;
        }

        // Compile text as a shader
        std::string str = stream.str();
        const char* src = str.c_str();
        shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, NULL);
        glCompileShader(shader);

        // Check for compilation errors
        GLint success;
        GLchar infoLog[512];
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 512, NULL, infoLog);
            std::cout << "Shader failed to compile: \n" << infoLog << std::endl;
        }
    }
    catch (std::ifstream::failure& e)
    {
        std::cout << "Shader (" << path << ") not found: " << e.what() << std::endl;
        assert(false);
    }

    return shader;
}

// Combine two compiled shaders into a program that can run on the GPU
GLuint CreateProgram(GLuint vs, GLuint fs)
{
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    // Check for linking errors
    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        program = GL_NONE;
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    return program;
}

// Graphics debug callback
void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char* message, const void* userParam)
{
    // ignore non-significant error/warning codes
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

    std::cout << "---------------" << std::endl;
    std::cout << "Debug message (" << id << "): " << message << std::endl;

    switch (source)
    {
    case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
    case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
    case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
    } std::cout << std::endl;

    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
    case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
    case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
    case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
    case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
    case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
    case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
    } std::cout << std::endl;

    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
    case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
    case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
    case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
    } std::cout << std::endl;
    std::cout << std::endl;
}

bool IsKeyDown(int key)
{
    return gKeysCurr[key] == GLFW_PRESS;
}

bool IsKeyUp(int key)
{
    return gKeysCurr[key] == GLFW_RELEASE;
}

bool IsKeyPressed(int key)
{
    return gKeysPrev[key] == GLFW_PRESS && gKeysCurr[key] == GLFW_RELEASE;
}

void Print(Matrix m)
{
    printf("%f %f %f %f\n", m.m0, m.m4, m.m8, m.m12);
    printf("%f %f %f %f\n", m.m1, m.m5, m.m9, m.m13);
    printf("%f %f %f %f\n", m.m2, m.m6, m.m10, m.m14);
    printf("%f %f %f %f\n\n", m.m3, m.m7, m.m11, m.m15);
}
