#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Mesh.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <array>

constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 720;
constexpr float SCREEN_ASPECT = SCREEN_WIDTH / (float)SCREEN_HEIGHT;

void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char* message, const void* userParam);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void error_callback(int error, const char* description);

GLuint CreateShader(GLint type, const char* path);
GLuint CreateProgram(GLuint vs, GLuint fs);

std::array<int, GLFW_KEY_LAST> gKeysCurr{}, gKeysPrev{};
bool IsKeyDown(int key);
bool IsKeyUp(int key);
bool IsKeyPressed(int key);

void Print(Matrix m);

enum Projection : int
{
    ORTHO,  // Orthographic, 2D
    PERSP   // Perspective,  3D
};

std::vector<Vector2> GenerateLines(std::vector<Vector2> verts)
{
    std::vector<Vector2> lines(verts.size() * 2);
    for (int i = 0; i < lines.size(); i = i + 2)
    {
        lines[i] = verts[i / 2];
        lines[i + 1] = verts[(i / 2 + 1) % verts.size()];
    }
    return lines;
}

std::vector<Vector2> GenerateMidpoints(std::vector<Vector2> verts)
{
    std::vector<Vector2> mids(verts.size());

    for (int i = 0; i < mids.size(); i++)
    {
        mids[i] = (verts[i] + verts[(i + 1) % mids.size()]) / 2.0f;
    }

    return mids;
}

std::vector<Vector2> GenerateSquares(int subdivisions)
{
    // iteration 1
    std::vector<Vector2> points
    {
        {-1.0f,  1.0f},
        { 1.0f,  1.0f},
        { 1.0f, -1.0f},
        {-1.0f, -1.0f}
    };

    std::vector<Vector2> result = points;
    for (int i = 0; i < subdivisions; i++)
    {
        points = GenerateMidpoints(points);
        for (Vector2 v : points)
        {
            result.push_back(v);
        }
    }

    return result;
}

std::vector<Vector2> GenerateLineShapes(std::vector<Vector2> verts, int sidesPerShape = 4)
{
    std::vector<Vector2> lines(verts.size() * 2);
    int i = 0;
    while (i < lines.size())
    {
        for (int j = 0; j < sidesPerShape * 2; j = j + 2)
        {
            lines[i + j] = verts[(i + j) / 2];
            lines[i + j + 1] = verts[((i + j) / 2 + 1) % sidesPerShape];
        }
        i = i + sidesPerShape;
    }
    return lines;
}

int main(void)
{
    glfwSetErrorCallback(error_callback);
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

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    //ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    // Vertex shaders:
    GLuint vs = CreateShader(GL_VERTEX_SHADER, "./assets/shaders/default.vert");
    GLuint vsPoints = CreateShader(GL_VERTEX_SHADER, "./assets/shaders/points.vert");
    GLuint vsLines = CreateShader(GL_VERTEX_SHADER, "./assets/shaders/lines.vert");
    GLuint vsVertexPositionColor = CreateShader(GL_VERTEX_SHADER, "./assets/shaders/vertex_color.vert");
    GLuint vsColorBufferColor = CreateShader(GL_VERTEX_SHADER, "./assets/shaders/buffer_color.vert");
    
    // Fragment shaders:
    GLuint fsLines = CreateShader(GL_FRAGMENT_SHADER, "./assets/shaders/lines.frag");
    GLuint fsUniformColor = CreateShader(GL_FRAGMENT_SHADER, "./assets/shaders/uniform_color.frag");
    GLuint fsVertexColor = CreateShader(GL_FRAGMENT_SHADER, "./assets/shaders/vertex_color.frag");
    GLuint fsTcoords = CreateShader(GL_FRAGMENT_SHADER, "./assets/shaders/tcoord_color.frag");
    GLuint fsNormals = CreateShader(GL_FRAGMENT_SHADER, "./assets/shaders/normal_color.frag");
    
    // Shader programs:
    GLuint shaderUniformColor = CreateProgram(vs, fsUniformColor);
    GLuint shaderVertexPositionColor = CreateProgram(vsVertexPositionColor, fsVertexColor);
    GLuint shaderVertexBufferColor = CreateProgram(vsColorBufferColor, fsVertexColor);
    GLuint shaderPoints = CreateProgram(vsPoints, fsVertexColor);
    GLuint shaderLines = CreateProgram(vsLines, fsLines);
    GLuint shaderTcoords = CreateProgram(vs, fsTcoords);
    GLuint shaderNormals = CreateProgram(vs, fsNormals);

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
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), nullptr);      // Describe the buffer
    glEnableVertexAttribArray(1);

    glBindVertexArray(GL_NONE);

    std::vector<Vector2> pointPositions(30000);
    std::vector<Vector3> pointColours(30000);
    for (int i = 1; i < pointPositions.size(); i++)
    {
        //pointPositions[i].x = Random(-1.0f, 1.0f);
        //pointPositions[i].y = Random(-1.0f, 1.0f);
        //pointColours[i] = colours[rand() % 3];

        Vector2 prev = pointPositions[i - 1];
        Vector2 curr = positions[rand() % 3];
        pointPositions[i] = (prev + curr) * 0.5f;

        float r = Random(0.0f, 1.0f);
        float g = Random(0.0f, 1.0f);
        float b = Random(0.0f, 1.0f);
        pointColours[i] = { r, g, b };
    }

    GLuint vaoPoints, pboPoints, cboPoints;
    glGenVertexArrays(1, &vaoPoints);
    glBindVertexArray(vaoPoints);

    glGenBuffers(1, &pboPoints);
    glBindBuffer(GL_ARRAY_BUFFER, pboPoints);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vector2) * pointPositions.size(), pointPositions.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(5, 2, GL_FLOAT, GL_FALSE, sizeof(Vector2), nullptr);
    glEnableVertexAttribArray(5);

    glGenBuffers(1, &cboPoints);
    glBindBuffer(GL_ARRAY_BUFFER, cboPoints);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3) * pointColours.size(), pointColours.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(13, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), nullptr);
    glEnableVertexAttribArray(13);

    glBindVertexArray(GL_NONE);

    std::vector<Vector2> linePositions = GenerateLineShapes(GenerateSquares(2));
    std::vector<Vector3> lineColors(linePositions.size());
    for (Vector3& c : lineColors)
        c = V3_ONE;

    GLuint vaoLines, pboLines, cboLines;
    glGenVertexArrays(1, &vaoLines);
    glBindVertexArray(vaoLines);

    glGenBuffers(1, &pboLines);
    glBindBuffer(GL_ARRAY_BUFFER, pboLines);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vector2) * linePositions.size(), linePositions.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vector2), nullptr);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &cboLines);
    glBindBuffer(GL_ARRAY_BUFFER, cboLines);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3) * lineColors.size(), lineColors.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), nullptr);
    glEnableVertexAttribArray(1);

    glBindVertexArray(GL_NONE);

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

    int object = 3;
    printf("Object %i\n", object + 1);

    Projection projection = PERSP;
    Vector3 camPos{ 0.0f, 0.0f, 5.0f };
    float fov = 75.0f * DEG2RAD;
    float left = -1.0f;
    float right = 1.0f;
    float top = 1.0f;
    float bottom = -1.0f;
    float near = 1.0f; // 1.0 for testing purposes. Usually 0.1f or 0.01f
    float far = 10.0f;

    // Whether we render the imgui demo widgets
    bool imguiDemo = false;

    Mesh shapeMesh, objMesh;
    CreateMesh(&shapeMesh, PLANE);
    CreateMesh(&objMesh, "assets/meshes/plane.obj");

    // Render looks weird cause this isn't enabled, but its causing unexpected problems which I'll fix soon!
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        // Change object when space is pressed
        if (IsKeyPressed(GLFW_KEY_SPACE))
        {
            ++object %= 5;
            printf("Object %i\n", object + 1);
        }

        if (IsKeyPressed(GLFW_KEY_I))
            imguiDemo = !imguiDemo;

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

        Matrix world = MatrixIdentity();
        Matrix view = LookAt(camPos, camPos - V3_FORWARD, V3_UP);
        Matrix proj = projection == ORTHO ?
            Ortho(left, right, bottom, top, near, far) :
            Perspective(fov, SCREEN_ASPECT, near, far);
        Matrix mvp = MatrixIdentity();
        GLint u_mvp = GL_NONE;

        GLuint shaderProgram = GL_NONE;

        switch (object + 1)
        {
        case 1:
            shaderProgram = shaderVertexBufferColor;
            glUseProgram(shaderProgram);
            world = s * r * t;
            mvp = world * view * proj;
            u_mvp = glGetUniformLocation(shaderProgram, "u_mvp");
            glUniformMatrix4fv(u_mvp, 1, GL_FALSE, ToFloat16(mvp).v);
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            break;

        case 2:
            shaderProgram = shaderVertexBufferColor;
            glUseProgram(shaderProgram);
            world = s * r * t;
            mvp = world * view * proj;
            u_mvp = glGetUniformLocation(shaderProgram, "u_mvp");
            glUniformMatrix4fv(u_mvp, 1, GL_FALSE, ToFloat16(mvp).v);
            glBindVertexArray(vao);
            glDrawArrays(GL_LINE_LOOP, 0, 3);
            break;

        case 3:
            shaderProgram = shaderPoints;
            glPointSize(1.0f);
            glUseProgram(shaderProgram);
            glBindVertexArray(vaoPoints);
            glDrawArrays(GL_POINTS, 0, pointPositions.size());
            break;

        case 4:
            shaderProgram = shaderLines;
            glUseProgram(shaderProgram);
            glLineWidth(4.0f);
            glBindVertexArray(vaoLines);
            glDrawArrays(GL_LINES, 0, linePositions.size());
            break;

        case 5:
            shaderProgram = shaderTcoords;
            glUseProgram(shaderProgram);
            world = MatrixIdentity();
            //world = RotateY(100.0f * time * DEG2RAD);
            mvp = world * view * proj;
            u_mvp = glGetUniformLocation(shaderProgram, "u_mvp");
            glUniformMatrix4fv(u_mvp, 1, GL_FALSE, ToFloat16(mvp).v);
            DrawMesh(shapeMesh);
            break;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        if (imguiDemo)
            ImGui::ShowDemoWindow();
        else
        {
            ImGui::SliderFloat3("Camera Position", &camPos.x, -10.0f, 10.0f);

            ImGui::RadioButton("Orthographic", (int*)&projection, 0); ImGui::SameLine();
            ImGui::RadioButton("Perspective", (int*)&projection, 1);

            ImGui::SliderFloat("Near", &near, -10.0f, 10.0f);
            ImGui::SliderFloat("Far", &far, -10.0f, 10.0f);
            if (projection == ORTHO)
            {
                ImGui::SliderFloat("Left", &left, -1.0f, -10.0f);
                ImGui::SliderFloat("Right", &right, 1.0f, 10.0f);
                ImGui::SliderFloat("Top", &top, 1.0f, 10.0f);
                ImGui::SliderFloat("Bottom", &bottom, -1.0f, -10.0f);
            }
            else if (projection == PERSP)
            {
                ImGui::SliderAngle("FoV", &fov, 10.0f, 90.0f);
            }
        }
        
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll and process events */
        memcpy(gKeysPrev.data(), gKeysCurr.data(), GLFW_KEY_LAST * sizeof(int));
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
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

void error_callback(int error, const char* description)
{
    printf("GLFW Error %d: %s\n", error, description);
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

// A (top)
//linePositions[0] = { -1.0f,  1.0f };
//linePositions[1] = {  1.0f,  1.0f };
//lineColors[0] = V3_UP;
//lineColors[1] = V3_UP;
//
//// B (right)
//linePositions[2] = linePositions[1];
//linePositions[3] = { 1.0f, -1.0f };
//lineColors[2] = V3_RIGHT;
//lineColors[3] = V3_RIGHT;
//
//// C (bottom)
//linePositions[4] = linePositions[3];
//linePositions[5] = { -1.0f, -1.0f };
//lineColors[4] = V3_FORWARD;
//lineColors[5] = V3_FORWARD;
//
//// D (left)
//linePositions[6] = linePositions[5];
//linePositions[7] = linePositions[0];
//lineColors[6] = V3_ONE;
//lineColors[7] = V3_ONE;

// GL_LINES: Vertices 0 and 1 are considered a line. Vertices 2 and 3 are considered a line. And so on.
//int lineVertexCount = SCREEN_WIDTH * 2;
//std::vector<Vector2> linePositions(lineVertexCount);
//std::vector<Vector3> lineColors(lineVertexCount);
//
//// Since lines are 2 vertices each, its easier to count up by 2 each iteration
//for (int i = 0; i < lineVertexCount; i += 2)
//{
//    float x = Remap(i, 0, lineVertexCount, -1.0f, 1.0f);
//    int A = i + 0;
//    int B = i + 1;
//    linePositions[A].x = x;
//    linePositions[B].x = x;
//    linePositions[A].y =  1.0f;
//    linePositions[B].y = -1.0f;
//    lineColors[A] = V3_RIGHT;
//    lineColors[B] = V3_UP;
//}

// 1 square has ***8*** vertices worth of lines (2 vertices per line * 4 lines)
//size_t lineVertexCount = 8;
//std::vector<Vector2> linePositions(lineVertexCount);
//std::vector<Vector3> lineColors(lineVertexCount);

//std::vector<Vector2> testVerts{
//    {-1.0f,  1.0f},
//    { 1.0f,  1.0f},
//    { 1.0f, -1.0f},
//    {-1.0f, -1.0f}
//};
//
//std::vector<Vector2> testLines = GenerateLines(testVerts);
