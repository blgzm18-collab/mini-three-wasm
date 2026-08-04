#include "shader.h"
#include <cstdio>

Shader::Shader(const char* vertexSrc, const char* fragmentSrc) {
    GLuint vs = compile(GL_VERTEX_SHADER, vertexSrc);
    GLuint fs = compile(GL_FRAGMENT_SHADER, fragmentSrc);

    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    GLint linked;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        char log[512];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        printf("Program link error:\n%s\n", log);
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
}

Shader::~Shader() {
    glDeleteProgram(program);
}

GLuint Shader::compile(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        printf("Shader compile error:\n%s\n", log);
    }

    return shader;
}

void Shader::use() const {
    glUseProgram(program);
}

GLint Shader::getAttribLocation(const char* name) const {
    return glGetAttribLocation(program, name);
}

GLint Shader::getUniformLocation(const char* name) const {
    return glGetUniformLocation(program, name);
}
