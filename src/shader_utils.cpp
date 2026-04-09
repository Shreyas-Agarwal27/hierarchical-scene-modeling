#include "shader_utils.h"

#include <glad/glad.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

unsigned int compileShaders(const char* vertexPath, const char* fragmentPath) {
    auto readCode = [](const char* path) {
        std::ifstream file(path);
        if (!file) {
            throw std::runtime_error("Failed to open file");
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    };

    std::string vCodeStr = readCode(vertexPath);
    std::string fCodeStr = readCode(fragmentPath);
    const char* vCode = vCodeStr.c_str();
    const char* fCode = fCodeStr.c_str();

    unsigned int vertex, fragment, program;
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vCode, NULL);
    glCompileShader(vertex);

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fCode, NULL);
    glCompileShader(fragment);

    program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return program;
}
