#ifndef GL_SHADER_HPP
#define GL_SHADER_HPP

#include <GL/glew.h>

namespace Nova::Renderer::OpenGL {

    std::string readFile(const std::string& filePath);

    GLuint loadRenderShader(const std::string& vertexPath, const std::string& fragmentPath);
    GLuint loadComputeShader(const std::string& computePath);
    
    GLuint compileShader(GLenum shaderType, const std::string& shaderCode);
    GLuint linkProgram(const std::initializer_list<GLuint>& shaderIDs);

} // namespace Nova::Renderer::OpenGL

#endif // GL_SHADER_HPP