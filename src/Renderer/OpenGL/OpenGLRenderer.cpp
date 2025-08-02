#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Renderer/OpenGL/OpenGLRenderer.hpp"
#include "Renderer/OpenGL/Shader.hpp"

using namespace Nova::Components;

namespace Nova::Renderer::OpenGL {

    void OpenGLRenderer::init(Nova::Scene& scene) {
        m_Scene = &scene;

        if(glewInit() != GLEW_OK) {
            std::cerr << "Failed to initialize GLEW\n";
        }

        std::string vertexPath = std::string(SHADER_DIR) + "/vertex.vert";
        std::string fragmentPath = std::string(SHADER_DIR) + "/fragment.frag";
        m_shaderProgram = loadRenderShader(vertexPath, fragmentPath);

        if (m_shaderProgram == 0) {
            std::cerr << "Failed to load or compile shaders!" << std::endl;
        }

        initFBO(m_ViewportWidth, m_ViewportHeight);
    }

    void OpenGLRenderer::initFBO(int width, int height) {        
        if (m_FBO) {
            glDeleteFramebuffers(1, &m_FBO);
            glDeleteTextures(1, &m_ColorTexture);
            glDeleteRenderbuffers(1, &m_DepthBuffer);
        }

        glGenFramebuffers(1, &m_FBO);
        glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

        glGenTextures(1, &m_ColorTexture);
        glBindTexture(GL_TEXTURE_2D, m_ColorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorTexture, 0);

        glGenRenderbuffers(1, &m_DepthBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, m_DepthBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_DepthBuffer);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cerr << "FBO not complete!\n";

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLRenderer::updateViewportSize(int width, int height) {
        if (width != m_ViewportWidth || height != m_ViewportHeight) {
            m_ViewportWidth = width;
            m_ViewportHeight = height;
            initFBO(width, height);

            if (m_Scene) {
                auto camEntity = m_Scene->getViewportCamera();
                if (camEntity != entt::null) {
                    auto& cam = m_Scene->registry().get<CameraComponent>(camEntity);
                    cam.m_AspectRatio = static_cast<float>(width) / static_cast<float>(height);
                }
            }
        }
    }

    void OpenGLRenderer::render() {
        // bind FBO
        glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
        glViewport(0,0,m_ViewportWidth,m_ViewportHeight);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.1f,0.1f,0.1f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        glUseProgram(m_shaderProgram);
        // Camera
        auto camEntity = m_Scene->getViewportCamera();
        if(camEntity != entt::null) {
            auto& cam = m_Scene->registry().get<CameraComponent>(camEntity);
            glm::mat4 view = cam.getViewMatrix();
            glm::mat4 proj = cam.getProjectionMatrix();
            glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram,"u_View"),1,GL_FALSE,glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram,"u_Projection"),1,GL_FALSE,glm::value_ptr(proj));
            glUniform3fv(glGetUniformLocation(m_shaderProgram,"u_CameraPos"),1,glm::value_ptr(cam.m_LookFrom));
        }
        glUniform3f(glGetUniformLocation(m_shaderProgram, "u_ObjectColor"), 1.0f, 0.0f, 0.0f);
        // Lights
        m_Scene->forEach<TransformComponent, LightComponent>([&](entt::entity id, TransformComponent& tf, LightComponent& lt){
            glm::vec3 pos = tf.GetTransform() * glm::vec4(0,0,0,1);
            glUniform3fv(glGetUniformLocation(m_shaderProgram,"u_LightPos"),1,glm::value_ptr(pos));
            glUniform3fv(glGetUniformLocation(m_shaderProgram,"u_LightColor"),1,glm::value_ptr(lt.m_Color));
            glUniform1f(glGetUniformLocation(m_shaderProgram,"u_LightIntensity"),lt.m_Intensity);
        });
        // Meshes
        m_Scene->forEach<TransformComponent, MeshComponent>([&](entt::entity id, TransformComponent& tf, MeshComponent& mesh){
            // upload VAO once per mesh or cache
            GLuint vao = uploadMesh(mesh);
            glm::mat4 model = tf.GetTransform();
            glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram,"u_Model"),1,GL_FALSE,glm::value_ptr(model));
            glBindVertexArray(vao);
            glDrawElements(GL_TRIANGLES, (GLsizei)mesh.m_Indices.size(), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        });
        glUseProgram(0);
        glBindFramebuffer(GL_FRAMEBUFFER,0);
    }

    GLuint OpenGLRenderer::uploadMesh(const Nova::Components::MeshComponent& mesh) {
        static std::unordered_map<const Nova::Components::MeshComponent*,GLuint> cache;
        auto it = cache.find(&mesh);
        if(it!=cache.end()) return it->second;
        GLuint vao,vbo,ibo;
        glGenVertexArrays(1,&vao);
        glBindVertexArray(vao);
        glGenBuffers(1,&vbo);
        glBindBuffer(GL_ARRAY_BUFFER,vbo);
        struct Vertex{glm::vec3 pos,nrm;};
        std::vector<Vertex> data;
        for(size_t i=0;i<mesh.m_Vertices.size();++i)
            data.push_back({mesh.m_Vertices[i],mesh.m_Normals[i]});
        glBufferData(GL_ARRAY_BUFFER,data.size()*sizeof(Vertex),data.data(),GL_STATIC_DRAW);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,pos));glEnableVertexAttribArray(0);
        glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,nrm));glEnableVertexAttribArray(1);
        glGenBuffers(1,&ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,mesh.m_Indices.size()*sizeof(unsigned),mesh.m_Indices.data(),GL_STATIC_DRAW);
        glBindVertexArray(0);
        cache[&mesh]=vao;
        return vao;
    }

    void OpenGLRenderer::destroy() {
        glDeleteVertexArrays(1, &m_VAO);
        glDeleteBuffers(1, &m_VBO);
        glDeleteProgram(m_shaderProgram);
    }
} // namespace Nova::Renderer::OpenGL
