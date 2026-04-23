#include "shader.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>

namespace {

std::string readFile(const char* path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error(std::string("Failed to open shader file: ") + path);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

unsigned int compileShader(unsigned int type, const std::string& source, const char* label) {
    const char* sourcePtr = source.c_str();
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &sourcePtr, nullptr);
    glCompileShader(shader);

    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        int infoLogLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
        std::string infoLog(static_cast<size_t>(infoLogLength), '\0');
        glGetShaderInfoLog(shader, infoLogLength, nullptr, infoLog.data());
        glDeleteShader(shader);
        throw std::runtime_error(std::string("Failed to compile ") + label + " shader:\n" + infoLog);
    }

    return shader;
}

}  // namespace

Shader::Shader(const char* vertexPath, const char* fragmentPath) : id_(0) {
    const std::string vertexCode = readFile(vertexPath);
    const std::string fragmentCode = readFile(fragmentPath);

    const unsigned int vertex = compileShader(GL_VERTEX_SHADER, vertexCode, "vertex");
    const unsigned int fragment = compileShader(GL_FRAGMENT_SHADER, fragmentCode, "fragment");

    id_ = glCreateProgram();
    glAttachShader(id_, vertex);
    glAttachShader(id_, fragment);
    glLinkProgram(id_);

    int success = 0;
    glGetProgramiv(id_, GL_LINK_STATUS, &success);
    if (!success) {
        int infoLogLength = 0;
        glGetProgramiv(id_, GL_INFO_LOG_LENGTH, &infoLogLength);
        std::string infoLog(static_cast<size_t>(infoLogLength), '\0');
        glGetProgramInfoLog(id_, infoLogLength, nullptr, infoLog.data());
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        glDeleteProgram(id_);
        throw std::runtime_error(std::string("Failed to link shader program:\n") + infoLog);
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

Shader::~Shader() {
    if (id_ != 0) {
        glDeleteProgram(id_);
    }
}

void Shader::use() const {
    glUseProgram(id_);
}

void Shader::setBool(const std::string& name, bool value) const {
    glUniform1i(glGetUniformLocation(id_, name.c_str()), static_cast<int>(value));
}

void Shader::setInt(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(id_, name.c_str()), value);
}

void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(id_, name.c_str()), value);
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const {
    glUniform3fv(glGetUniformLocation(id_, name.c_str()), 1, glm::value_ptr(value));
}

void Shader::setMat4(const std::string& name, const glm::mat4& value) const {
    glUniformMatrix4fv(glGetUniformLocation(id_, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}
