#pragma once
#include <glad/glad.h>

GLuint CreateShader(GLint type, const char* path);
GLuint CreateProgram(GLuint vs, GLuint fs);

void SendFloat(GLuint shader, const char* name, float value);