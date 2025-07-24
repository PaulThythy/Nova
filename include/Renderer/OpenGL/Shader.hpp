#ifndef SHADER_HPP
#define SHADER_HPP

#include <GL/glew.h>

namespace Nova {
    namespace Renderer {
        namespace OpenGL {

            std::string readFile(const std::string& filePath);

            GLuint loadRenderShader(const std::string& vertexPath, const std::string& fragmentPath);
            GLuint loadComputeShader(const std::string& computePath);
            
            GLuint compileShader(GLenum shaderType, const std::string& shaderCode);
            GLuint linkProgram(const std::initializer_list<GLuint>& shaderIDs);

        } // namespace OpenGL
    } // namespace Renderer
} // namespace Nova

#endif // SHADER_HPP