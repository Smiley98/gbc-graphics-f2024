#include "Shader.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>

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

GLint GetLocation(GLuint shader, const char* name)
{
    // Future improvement: do some validation
    // ie if glGetUniformLocation returns -1, crash the program?
    GLint location = glGetUniformLocation(shader, name);
    assert(location != -1, "Shader variable (name) does not exist!");
    return glGetUniformLocation(shader, name);
}

void SendInt(GLuint shader, const char* name, int value)
{
    GLint location = GetLocation(shader, name);
    glUniform1i(location, value);
}

void SendFloat(GLuint shader, const char* name, float value)
{
    GLint location = GetLocation(shader, name);
    glUniform1f(location, value);
}

void SendVec2(GLuint shader, const char* name, Vector2 value)
{
    GLint location = GetLocation(shader, name);
    glUniform2f(location, value.x, value.y);
}

void SendVec3(GLuint shader, const char* name, Vector3 value)
{
    GLint location = GetLocation(shader, name);
    glUniform3f(location, value.x, value.y, value.z);
}

void SendVec4(GLuint shader, const char* name, Vector4 value)
{
    GLint location = GetLocation(shader, name);
    glUniform4f(location, value.x, value.y, value.z, value.w);
}

void SendMat3(GLuint shader, const char* name, Matrix value)
{
    GLint location = GetLocation(shader, name);
    float9 v = ToFloat9(value);
    glUniformMatrix3fv(location, 1, GL_FALSE, v.v);
}

void SendMat4(GLuint shader, const char* name, Matrix value)
{
    GLint location = GetLocation(shader, name);
    float16 v = ToFloat16(value);
    glUniformMatrix4fv(location, 1, GL_FALSE, v.v);
}

void SendMat4Array(GLuint shader, const char* name, Matrix* values, int count)
{
    GLint location = GetLocation(shader, name);
    float16* v = new float16[count];
    for (int i = 0; i < 100; i++)
    {
        v[i] = ToFloat16(values[i]);
    }
    glUniformMatrix4fv(location, count, GL_FALSE, (float*)v);
    delete[] v;
}
