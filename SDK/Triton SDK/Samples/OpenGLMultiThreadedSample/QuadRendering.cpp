// Copyright (c) 2011-2018 Sundog Software LLC. All rights reserved worldwide.
#include <cassert>
#include <string>
#include <fstream>
#include <iostream>

#include "QuadRendering.h"

using namespace std;

static const GLfloat quad_vertices[] = {
    -1, -1, 0, 0
    , +1, -1, 1, 0
    , +1, +1, 1, 1
    , -1, +1, 0, 1
};

static const GLushort quad_indices[] = {
    0, 1, 2
    , 0, 2, 3
};


void createQuadVB(GLuint& quadVB)
{
    glGenBuffers(1, &quadVB);
    glBindBuffer(GL_ARRAY_BUFFER, quadVB);
    const int sizeVB = sizeof(float) * 4 * 4;
    glBufferData(GL_ARRAY_BUFFER, sizeVB, quad_vertices, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeVB, quad_vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void createQuadIB(GLuint& quadIB)
{
    glGenBuffers(1, &quadIB);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadIB);
    const int sizeIB = sizeof(GLushort) * 6;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeIB, 0, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeIB, quad_indices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


bool compiled(GLuint shader)
{
    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        GLsizei log_length = 0;
        GLchar message[1024];
        glGetShaderInfoLog(shader, 1024, &log_length, message);

        std::string errMessage(message);
        std::cout << "Compilation Error:" << std::endl;
        std::cout << errMessage << std::endl;
        return false;
    }
    return true;
}

void CreateQuadProgram(GLuint& quadProgram)
{
    std::ifstream vsStream("quad_vs.glsl");
    std::string srcVS((std::istreambuf_iterator<char>(vsStream)), std::istreambuf_iterator<char>());

    std::ifstream psStream("quad_ps.glsl");
    std::string srcPS((std::istreambuf_iterator<char>(psStream)), std::istreambuf_iterator<char>());

    const char *vs_src[1] = { srcVS.c_str() };
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, vs_src, NULL);
    glCompileShader(vs);
    assert(compiled(vs));

    const char *ps_src[1] = { srcPS.c_str() };
    GLuint ps = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(ps, 1, ps_src, NULL);
    glCompileShader(ps);
    assert(compiled(ps));

    quadProgram = glCreateProgram();
    glAttachShader(quadProgram, vs);
    glAttachShader(quadProgram, ps);

    glLinkProgram(quadProgram);

    GLint program_linked;
    glGetProgramiv(quadProgram, GL_LINK_STATUS, &program_linked);
    assert(program_linked);

    //glDeleteShader(vs);
    //glDeleteShader(ps);
}