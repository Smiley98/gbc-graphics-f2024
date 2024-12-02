#pragma once
#include <glad/glad.h>
#include "Math.h"

GLuint CreateShader(GLint type, const char* path);
GLuint CreateProgram(GLuint vs, GLuint fs);

void SendInt(GLuint shader, const char* name, int value);
void SendFloat(GLuint shader, const char* name, float value);
void SendVec2(GLuint shader, const char* name, Vector2 value);
void SendVec3(GLuint shader, const char* name, Vector3 value);
void SendVec4(GLuint shader, const char* name, Vector4 value);
void SendMat3(GLuint shader, const char* name, Matrix value);
void SendMat4(GLuint shader, const char* name, Matrix value);
