#include "mesh.h"

Mesh::Mesh(float* vertices, int vertexCount, Shader* shader)
    : vertexCount(vertexCount), shader(shader)
{
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(float), vertices, GL_STATIC_DRAW);
}

Mesh::~Mesh() {
    glDeleteBuffers(1, &vbo);
}

void Mesh::draw() {
    shader->use();

    // Send model matrix
    Mat4 model = transform.getModelMatrix();
    GLint modelLoc = shader->getUniformLocation("model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model.data);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    GLint posLoc = shader->getAttribLocation("position");
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glDrawArrays(GL_TRIANGLES, 0, vertexCount / 3);

    glDisableVertexAttribArray(posLoc);
}
