#pragma once
#include <string>
#include <GLES2/gl2.h>

class Shader {
public:
    Shader(const char* vertexSrc, const char* fragmentSrc);
    ~Shader();

    void use() const;
    GLint getAttribLocation(const char* name) const;
    GLint getUniformLocation(const char* name) const;

private:
    GLuint program;

    GLuint compile(GLenum type, const char* src);
};
