#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Mesh.h"
#include "Shader.h"

// TODO -- Texture.h & Texture.cpp during lab, show result next lexture
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include <cstdlib>
#include <iostream>
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

struct Pixel
{
    stbi_uc r = 255;
    stbi_uc g = 255;
    stbi_uc b = 255;
    stbi_uc a = 255;
};

GLuint CreateTexture(const char* texturePath)
{
    // Step 1: Load image from disk to CPU
    int texWidth = 0;
    int texHeight = 0;
    int texChannels = 0;
    stbi_uc* pixels = stbi_load(texturePath, &texWidth, &texHeight, &texChannels, 0);

    // Note that all shaders that sample textures do .xyz to discard alpha values
    GLint format = texChannels == 3 ? GL_RGB : GL_RGBA;
    assert(texChannels >= 3);

    // Step 2: Upload image from CPU to GPU
    GLuint tex = GL_NONE;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, format, texWidth, texHeight, 0, format, GL_UNSIGNED_BYTE, pixels);
    stbi_image_free(pixels);

    return tex;
}

// Extra practice: Make a DestroyTexture function

GLuint CreateSkybox(const char* skyboxPath[6])
{
    GLuint texSkybox = GL_NONE;
    glGenTextures(1, &texSkybox);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texSkybox);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_set_flip_vertically_on_load(false);
    for (int i = 0; i < 6; i++)
    {
        int w, h, c;
        stbi_uc* pixels = stbi_load(skyboxPath[i], &w, &h, &c, 0);
        GLint format = c == 3 ? GL_RGB : GL_RGBA;
        assert(c >= 3);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, pixels);
        stbi_image_free(pixels);
    }
    stbi_set_flip_vertically_on_load(true);
    glBindTexture(GL_TEXTURE_CUBE_MAP, GL_NONE);

    return texSkybox;
}

// Extra practice: Make a DestroySkybox function

void DrawSkybox(GLuint skybox, GLuint shader, const Mesh& cube, Matrix view, Matrix proj)
{
    // Skybox begin
    shader = shader;
    glUseProgram(shader);

    Matrix viewSky = view;
    viewSky.m12 = viewSky.m13 = viewSky.m14 = 0.0f;
    Matrix mvp = viewSky * proj;

    SendMat4(shader, "u_mvp", mvp);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox);
    glDepthMask(GL_FALSE);
    DrawMesh(cube);
    glDepthMask(GL_TRUE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, GL_NONE);
    // Skybox end
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
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    // Vertex shaders:
    GLuint vs = CreateShader(GL_VERTEX_SHADER, "./assets/shaders/default.vert");
    GLuint vsSkybox = CreateShader(GL_VERTEX_SHADER, "./assets/shaders/skybox.vert");
    GLuint vsPoints = CreateShader(GL_VERTEX_SHADER, "./assets/shaders/points.vert");
    GLuint vsLines = CreateShader(GL_VERTEX_SHADER, "./assets/shaders/lines.vert");
    GLuint vsVertexPositionColor = CreateShader(GL_VERTEX_SHADER, "./assets/shaders/vertex_color.vert");
    GLuint vsColorBufferColor = CreateShader(GL_VERTEX_SHADER, "./assets/shaders/buffer_color.vert");
    GLuint vsAsteroids = CreateShader(GL_VERTEX_SHADER, "./assets/shaders/asteroids.vert");
    
    // Fragment shaders:
    GLuint fsSkybox = CreateShader(GL_FRAGMENT_SHADER, "./assets/shaders/skybox.frag");
    GLuint fsLines = CreateShader(GL_FRAGMENT_SHADER, "./assets/shaders/lines.frag");
    GLuint fsUniformColor = CreateShader(GL_FRAGMENT_SHADER, "./assets/shaders/uniform_color.frag");
    GLuint fsVertexColor = CreateShader(GL_FRAGMENT_SHADER, "./assets/shaders/vertex_color.frag");
    GLuint fsTcoords = CreateShader(GL_FRAGMENT_SHADER, "./assets/shaders/tcoord_color.frag");
    GLuint fsNormals = CreateShader(GL_FRAGMENT_SHADER, "./assets/shaders/normal_color.frag");
    GLuint fsTexture = CreateShader(GL_FRAGMENT_SHADER, "./assets/shaders/texture_color.frag");
    GLuint fsTextureMix = CreateShader(GL_FRAGMENT_SHADER, "./assets/shaders/texture_color_mix.frag");
    GLuint fsPhong = CreateShader(GL_FRAGMENT_SHADER, "./assets/shaders/phong.frag");
    GLuint fsReflect = CreateShader(GL_FRAGMENT_SHADER, "./assets/shaders/reflect.frag");
    GLuint fsRefract = CreateShader(GL_FRAGMENT_SHADER, "./assets/shaders/refract.frag");
    GLuint fsAsteroids = CreateShader(GL_FRAGMENT_SHADER, "./assets/shaders/asteroids.frag");
    
    // Shader programs:
    GLuint shaderUniformColor = CreateProgram(vs, fsUniformColor);
    GLuint shaderVertexPositionColor = CreateProgram(vsVertexPositionColor, fsVertexColor);
    GLuint shaderVertexBufferColor = CreateProgram(vsColorBufferColor, fsVertexColor);
    GLuint shaderPoints = CreateProgram(vsPoints, fsVertexColor);
    GLuint shaderLines = CreateProgram(vsLines, fsLines);
    GLuint shaderTcoords = CreateProgram(vs, fsTcoords);
    GLuint shaderNormals = CreateProgram(vs, fsNormals);
    GLuint shaderTexture = CreateProgram(vs, fsTexture);
    GLuint shaderTextureMix = CreateProgram(vs, fsTextureMix);
    GLuint shaderSkybox = CreateProgram(vsSkybox, fsSkybox);
    GLuint shaderPhong = CreateProgram(vs, fsPhong);
    GLuint shaderReflect = CreateProgram(vs, fsReflect);
    GLuint shaderRefract = CreateProgram(vs, fsRefract);
    GLuint shaderAsteroids = CreateProgram(vsAsteroids, fsAsteroids);

    // Our obj file defines tcoords as 0 = bottom, 1 = top, but OpenGL defines as 0 = top 1 = bottom.
    // Flipping our image vertically is the best way to solve this as it ensures a "one-stop" solution (rather than an in-shader solution).
    stbi_set_flip_vertically_on_load(true);

    // Note that generating an image ourselves is intuitive in OpenGL's space since it matches 2D array coordinates!
    int texGradientWidth = 256;
    int texGradientHeight = 256;
    Pixel* pixelsGradient = (Pixel*)malloc(texGradientWidth * texGradientHeight * sizeof(Pixel));
    for (int y = 0; y < texGradientHeight; y++)
    {
        for (int x = 0; x < texGradientWidth; x++)
        {
            float u = x / (float)texGradientWidth;
            float v = y / (float)texGradientHeight;

            Pixel pixel;
            pixel.r = u * 255.0f;
            pixel.g = v * 255.0f;
            pixel.b = 255;
            pixel.a = 255;

            pixelsGradient[y * texGradientWidth + x] = pixel;
        }
    }

    GLuint texHead = CreateTexture("./assets/textures/head.png");
    GLuint texAsteroid = CreateTexture("./assets/textures/asteroid.png");

    // CreateTexture could be improved if we feed it pixels instead of a file path, so we can use it to upload our manually created gradient to the GPU
    // Even better if we make a function overload
    GLuint texGradient = GL_NONE;
    glGenTextures(1, &texGradient);
    glBindTexture(GL_TEXTURE_2D, texGradient);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texGradientWidth, texGradientHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsGradient);
    free(pixelsGradient);
    pixelsGradient = nullptr;

    const char* skyboxArcticPath[6] =
    {
        "./assets/textures/arctic_x+.jpg",
        "./assets/textures/arctic_x-.jpg",
        "./assets/textures/arctic_y+.jpg",
        "./assets/textures/arctic_y-.jpg",
        "./assets/textures/arctic_z+.jpg",
        "./assets/textures/arctic_z-.jpg"
    };
    const char* skyboxSpacePath[6] =
    {
        "./assets/textures/sky_x+.png",
        "./assets/textures/sky_x-.png",
        "./assets/textures/sky_y+.png",
        "./assets/textures/sky_y-.png",
        "./assets/textures/sky_z+.png",
        "./assets/textures/sky_z-.png"
    };
    GLuint texSkyboxArctic = CreateSkybox(skyboxArcticPath);
    GLuint texSkyboxSpace = CreateSkybox(skyboxSpacePath);

    int object = 4;
    printf("Object %i\n", object + 1);

    Projection projection = PERSP;
    Vector3 camPos{ 0.0f, 0.0f, 5.0f };
    float fov = 75.0f * DEG2RAD;
    float left = -1.0f;
    float right = 1.0f;
    float top = 1.0f;
    float bottom = -1.0f;
    float near = 0.001f; // 1.0 for testing purposes. Usually 0.1f or 0.01f
    float far = 10.0f;

    // Whether we render the imgui demo widgets
    bool imguiDemo = false;
    bool texToggle = false;
    bool camToggle = false;

    Mesh headMesh, asteroidMesh, cubeMesh, sphereMesh;
    CreateMesh(&headMesh, "assets/meshes/head.obj");
    CreateMesh(&asteroidMesh, "assets/meshes/asteroid.obj");
    CreateMesh(&cubeMesh, CUBE);
    CreateMesh(&sphereMesh, SPHERE);

    float objectPitch = 0.0f;
    float objectYaw = 0.0f;
    Vector3 objectPosition = V3_ZERO;
    float objectSpeed = 10.0f;

    Vector3 lightPosition = { 5.0f, 5.0f, 0.0f };
    Vector3 lightColor = { 1.0f, 0.5f, 0.0f };
    float lightRadius = 1.0f;
    float lightAngle = 90.0f * DEG2RAD;

    float ambientFactor = 0.25f;
    float diffuseFactor = 1.0f;
    float specularPower = 64.0f;
    float refractiveIndex = 1.52f; // 1.52 = glass

    // Render looks weird cause this isn't enabled, but its causing unexpected problems which I'll fix soon!
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    float timePrev = glfwGetTime();
    float timeCurr = glfwGetTime();
    float dt = 0.0f;

    double pmx = 0.0, pmy = 0.0, mx = 0.0, my = 0.0;
    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        float time = glfwGetTime();
        timePrev = time;

        pmx = mx; pmy = my;
        glfwGetCursorPos(window, &mx, &my);
        Vector2 mouseDelta = { mx - pmx, my - pmy };

        // Change object when space is pressed
        if (IsKeyPressed(GLFW_KEY_TAB))
        {
            ++object %= 5;
            printf("Object %i\n", object + 1);
        }

        if (IsKeyPressed(GLFW_KEY_I))
            imguiDemo = !imguiDemo;

        if (IsKeyPressed(GLFW_KEY_T))
            texToggle = !texToggle;

        if (IsKeyPressed(GLFW_KEY_C))
        {
            camToggle = !camToggle;
            if (camToggle)
            {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
            else
            {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }

        Matrix objectRotation = ToMatrix(FromEuler(objectPitch * DEG2RAD, objectYaw * DEG2RAD, 0.0f));
        Matrix objectTranslation = Translate(objectPosition);

        Vector3 objectRight = { objectRotation.m0, objectRotation.m1, objectRotation.m2 };
        Vector3 objectUp = { objectRotation.m4, objectRotation.m5, objectRotation.m6 };
        Vector3 objectForward = { objectRotation.m8, objectRotation.m9, objectRotation.m10 };
        Matrix objectMatrix = objectRotation * objectTranslation;

        float objectDelta = objectSpeed * dt;
        float mouseScale = 1.0f;
        if (!camToggle)
        {
            objectDelta = 0.0f;
            mouseScale = 0.0f;
        }
        objectPitch += mouseDelta.y * mouseScale;
        objectYaw += mouseDelta.x * mouseScale;
        if (IsKeyDown(GLFW_KEY_W))
        {
            objectPosition += objectForward * objectDelta;
;       }
        if (IsKeyDown(GLFW_KEY_S))
        {
            objectPosition -= objectForward * objectDelta;
        }
        if (IsKeyDown(GLFW_KEY_A))
        {
            objectPosition -= objectRight * objectDelta;
        }
        if (IsKeyDown(GLFW_KEY_D))
        {
            objectPosition += objectRight * objectDelta;
        }
        if (IsKeyDown(GLFW_KEY_LEFT_SHIFT))
        {
            objectPosition -= objectUp * objectDelta;
        }
        if (IsKeyDown(GLFW_KEY_SPACE))
        {
            objectPosition += objectUp * objectDelta;
        }

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Matrix rotationX = RotateX(100.0f * time * DEG2RAD);
        Matrix rotationY = RotateY(100.0f * time * DEG2RAD);

        Matrix normal = MatrixIdentity();
        Matrix world = MatrixIdentity();
        Matrix view = LookAt(camPos, camPos - V3_FORWARD, V3_UP);
        Matrix proj = projection == ORTHO ? Ortho(left, right, bottom, top, near, far) : Perspective(fov, SCREEN_ASPECT, near, far);
        Matrix mvp = MatrixIdentity();

        GLuint shaderProgram = GL_NONE;
        GLuint textureTest = texToggle ? texGradient : texHead;

        switch (object + 1)
        {
        // Left side: object with texture applied to it
        // Right side: the coordinates our object uses to sample its texture
        case 1:
            shaderProgram = shaderTexture;
            glUseProgram(shaderProgram);
            world = objectMatrix;
            mvp = world * view * proj;

            SendMat4(shaderProgram, "u_mvp", mvp);
            SendInt(shaderProgram, "u_tex", 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureTest);
            DrawMesh(headMesh);

            shaderProgram = shaderTcoords;
            glUseProgram(shaderProgram);
            world = rotationX * Translate(2.5f, 0.0f, 0.0f);
            mvp = world * view * proj;
            SendMat4(shaderProgram, "u_mvp", mvp);
            DrawMesh(headMesh);
            break;

        // Interpolating (lerping) between 2 textures:
        case 2:
            shaderProgram = shaderTextureMix;
            glUseProgram(shaderProgram);
            
            mvp = world * view * proj;
            SendMat4(shaderProgram, "u_mvp", mvp);
            SendFloat(shaderProgram, "u_t", cosf(time) * 0.5f + 0.5f);
            
            SendInt(shaderProgram, "u_tex0", 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texGradient);
            
            SendInt(shaderProgram, "u_tex1", 1);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, texHead);
            
            DrawMesh(headMesh);
            break;

        // Phong
        case 3:
            shaderProgram = shaderPhong;
            glUseProgram(shaderProgram);
            world = objectMatrix;
            mvp = world * view * proj;
            normal = Transpose(Invert(world));
            
            SendMat3(shaderProgram, "u_normal", normal);
            SendMat4(shaderProgram, "u_world", world);
            SendMat4(shaderProgram, "u_mvp", mvp);
            
            SendVec3(shaderProgram, "u_cameraPosition", camPos);
            SendVec3(shaderProgram, "u_lightPosition", lightPosition);
            SendVec3(shaderProgram, "u_lightDirection", Direction(lightAngle));
            SendVec3(shaderProgram, "u_lightColor", lightColor);
            SendFloat(shaderProgram, "u_lightRadius", lightRadius);

            SendFloat(shaderProgram, "u_ambientFactor", ambientFactor);
            SendFloat(shaderProgram, "u_diffuseFactor", diffuseFactor);
            SendFloat(shaderProgram, "u_specularPower", specularPower);
            
            DrawMesh(sphereMesh);
            
            // Visualize light as wireframe
            shaderProgram = shaderUniformColor;
            glUseProgram(shaderProgram);
            world = Scale(V3_ONE * lightRadius) * Translate(lightPosition);
            mvp = world * view * proj;

            SendMat4(shaderProgram, "u_mvp", mvp);
            SendVec3(shaderProgram, "u_color", lightColor);

            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            DrawMesh(sphereMesh);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            break;

        // Skybox + environment mapping!
        // Extra practice 1 - Add an imgui slider float from 1.0 to 3.0 that changes the refractive index (u_ratio)
        // Extra practice 2 - Add FPS camera to see all 6 faces of the skybox!
        // Extra practice 3 - Add FPS controls to objects & make a key to cycle between controlling reflected vs refracted object
        case 4:
            DrawSkybox(texSkyboxArctic, shaderSkybox, cubeMesh, view, proj);

        // Reflect begin
            shaderProgram = shaderReflect;
            glUseProgram(shaderProgram);

            world = Translate(-2.0f, 0.0f, 0.0f);
            mvp = world * view * proj;
            normal = Transpose(Invert(world));

            SendMat3(shaderProgram, "u_normal", normal);
            SendMat4(shaderProgram, "u_world", world);
            SendMat4(shaderProgram, "u_mvp", mvp);
            SendVec3(shaderProgram, "u_cameraPosition", camPos);

            glBindTexture(GL_TEXTURE_CUBE_MAP, texSkyboxArctic);
            DrawMesh(cubeMesh);
            glBindTexture(GL_TEXTURE_CUBE_MAP, GL_NONE);
        // Reflect end

        // Refract begin
            shaderProgram = shaderRefract;
            glUseProgram(shaderProgram);

            world = Translate(2.0f, 0.0f, 0.0f);
            mvp = world * view * proj;
            normal = Transpose(Invert(world));

            SendMat3(shaderProgram, "u_normal", normal);
            SendMat4(shaderProgram, "u_world", world);
            SendMat4(shaderProgram, "u_mvp", mvp);
            SendVec3(shaderProgram, "u_cameraPosition", camPos);
            SendFloat(shaderProgram, "u_ratio", 1.00f / refractiveIndex);

            glBindTexture(GL_TEXTURE_CUBE_MAP, texSkyboxArctic);
            DrawMesh(cubeMesh);
            glBindTexture(GL_TEXTURE_CUBE_MAP, GL_NONE);
        // Refract end
            break;

        // Applies a texture to our object
        case 5:
            DrawSkybox(texSkyboxSpace, shaderSkybox, cubeMesh, view, proj);

            shaderProgram = shaderAsteroids;
            glUseProgram(shaderProgram);
            mvp = world * view * proj;

            SendMat4(shaderProgram, "u_mvp", mvp);
            SendInt(shaderProgram, "u_tex", 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texAsteroid);
            DrawMesh(asteroidMesh);
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
            ImGui::SliderFloat3("Light Position", &lightPosition.x, -10.0f, 10.0f);
            ImGui::SliderFloat("Light Radius", &lightRadius, 0.25f, 5.0f);
            ImGui::SliderAngle("Light Angle", &lightAngle);

            ImGui::SliderFloat("Ambient", &ambientFactor, 0.0f, 1.0f);
            ImGui::SliderFloat("Diffuse", &diffuseFactor, 0.0f, 1.0f);
            ImGui::SliderFloat("Specular", &specularPower, 8.0f, 256.0f);

            ImGui::SliderFloat("Refractive Index", &refractiveIndex, 1.0f, 3.0f);

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
        timeCurr = glfwGetTime();
        dt = timeCurr - timePrev;

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
