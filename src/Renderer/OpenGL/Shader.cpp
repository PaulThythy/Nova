#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

#include "Renderer/OpenGL/Shader.hpp"

namespace Nova {
    namespace Renderer {
        namespace OpenGL {

            std::string readFile(const std::string& filePath) {
                std::ifstream file(filePath);
                if (!file.is_open()) 
                {
                    std::cerr << "Failed to open shader file: " << filePath << std::endl;
                    return "";
                }
                std::stringstream buffer;
                buffer << file.rdbuf();
                file.close();
                return buffer.str();
            }

            GLuint compileShader(GLenum shaderType, const std::string& shaderCode) {
                if (shaderCode.empty())
                    return 0;

                const char* sourcePtr = shaderCode.c_str();
                GLuint shaderID = glCreateShader(shaderType);
                glShaderSource(shaderID, 1, &sourcePtr, nullptr);
                glCompileShader(shaderID);

                GLint compileStatus;
                glGetShaderiv(shaderID, GL_COMPILE_STATUS, &compileStatus);
                if (compileStatus == GL_FALSE) {
                    GLint logLength;
                    glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &logLength);

                    std::string log(logLength, '\0');
                    glGetShaderInfoLog(shaderID, logLength, nullptr, &log[0]);
                    std::cerr << "Shader compilation error:\n" << log << std::endl;

                    glDeleteShader(shaderID);
                    return 0;
                }
                return shaderID;
            }

            GLuint linkProgram(const std::initializer_list<GLuint>& shaderIDs) {
                GLuint programID = glCreateProgram();

                for (auto sid : shaderIDs) 
                {
                    if (sid != 0)
                        glAttachShader(programID, sid);
                }

                glLinkProgram(programID);

                GLint linkStatus;
                glGetProgramiv(programID, GL_LINK_STATUS, &linkStatus);
                if (linkStatus == GL_FALSE)
                {
                    GLint logLength;
                    glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &logLength);

                    std::string log(logLength, '\0');
                    glGetProgramInfoLog(programID, logLength, nullptr, &log[0]);
                    std::cerr << "Program linking error:\n" << log << std::endl;

                    glDeleteProgram(programID);
                    return 0;
                }

                for (auto sid : shaderIDs)
                {
                    if (sid != 0)
                    {
                        glDetachShader(programID, sid);
                        glDeleteShader(sid);
                    }
                }

                return programID;
            }

            GLuint loadRenderShader(const std::string& vertexPath, const std::string& fragmentPath) {
                std::string vertexCode = readFile(vertexPath);
                GLuint vertexID = compileShader(GL_VERTEX_SHADER, vertexCode);
                if (!vertexID)
                    return 0;

                std::string fragmentCode = readFile(fragmentPath);
                GLuint fragmentID = compileShader(GL_FRAGMENT_SHADER, fragmentCode);
                if (!fragmentID)
                    return 0;

                GLuint programID = linkProgram({ vertexID, fragmentID });
                return programID;
            }

            GLuint loadComputeShader(const std::string& computePath) {
                std::string computeCode = readFile(computePath);
                GLuint computeID = compileShader(GL_COMPUTE_SHADER, computeCode);
                if (!computeID)
                    return 0;

                GLuint programID = linkProgram({ computeID });
                return programID;
            }

        } // namespace OpenGL
    } // namespace Renderer
} // namespace Nova