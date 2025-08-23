#ifndef GL_SHADER_HPP
#define GL_SHADER_HPP

#include <GL/glew.h>

namespace Nova::Renderer::OpenGL {

    std::string ReadFile(const std::string& filePath);

    GLuint LoadRenderShader(const std::string& vertexPath, const std::string& fragmentPath);
    GLuint LoadComputeShader(const std::string& computePath);
    
    GLuint CompileShader(GLenum shaderType, const std::string& shaderCode);
    GLuint LinkProgram(const std::initializer_list<GLuint>& shaderIDs);

} // namespace Nova::Renderer::OpenGL

#endif // GL_SHADER_HPP