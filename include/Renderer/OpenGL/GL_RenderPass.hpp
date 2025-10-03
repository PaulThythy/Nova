#ifndef GL_RENDER_PASS_HPP
#define GL_RENDER_PASS_HPP

#include "Scene/Scene.hpp"
#include "Renderer/OpenGL/GL_RenderPassCtx.hpp"
#include "Math/AABB.hpp"

namespace Nova::Renderer::OpenGL {
	struct IGL_RenderPass {
		virtual ~IGL_RenderPass() = default;
		virtual void Init();
		virtual void Destroy();
		virtual void Execute(const GL_RenderPassCtx& ctx) = 0;
	};

	inline void IGL_RenderPass::Init() {}
	inline void IGL_RenderPass::Destroy() {}

	struct GL_DepthPrePass : public IGL_RenderPass {
		void Init() override;
		void Destroy() override;
		void Execute(const GL_RenderPassCtx& ctx) override;
		
		unsigned m_Program = 0;
    };

	struct GL_GBufferPass : public IGL_RenderPass {
		void Init() override;
		void Destroy() override;
		void Execute(const GL_RenderPassCtx& ctx) override;

		unsigned m_Program = 0;
	};

	struct GL_LightCullingPass : public IGL_RenderPass {
		void Init() override;
		void Destroy() override;
		void Execute(const GL_RenderPassCtx& ctx) override;

		unsigned m_Program = 0;

		int m_ClusterDimX = 16;			// 16 slides in width (screen)
		int m_ClusterDimY = 9;			// 9 slides in height (screen)
		int m_ClusterDimZ = 24; 		// 24 slides in depth

		unsigned m_LightsSSBO = 0;		//binding=0
		unsigned m_ClustersSSBO = 0;	//binding=1
		unsigned m_GridSSBO = 0;		//binding=2
		unsigned m_IndexListSSBO = 0;	//binding=3	

		int m_MaxLightsPerCluster = 128;

		std::vector<Math::AABB> m_ClusterAABBs; // view space AABBs

		size_t GetNumClusters() const {
            return size_t(m_ClusterDimX) * size_t(m_ClusterDimY) * size_t(m_ClusterDimZ);
        }

		void BuildClusterAABBs_ViewSpace(const GL_RenderPassCtx& ctx, std::vector<Math::AABB>& out);
		float LinearToNDC_Z(float zEye, float n, float f, const glm::mat4& proj);
		void UploadOrResizeBuffers(const GL_RenderPassCtx& ctx, int numClusters);
	};
	
	struct GL_LightingPass : public IGL_RenderPass {
		void Init() override;
		void Destroy() override;
		void Execute(const GL_RenderPassCtx& ctx) override;

		unsigned m_Program = 0;
	};

	struct GL_PostProcessPass : public IGL_RenderPass {
		void Init() override;
		void Destroy() override;
		void Execute(const GL_RenderPassCtx& ctx) override;

		unsigned m_Program = 0;
	};

	struct GL_OutlinePass : public IGL_RenderPass {
		void Init() override;
		void Destroy() override;
		void Execute(const GL_RenderPassCtx& ctx) override;

		unsigned m_Program = 0;
	};

} // namespace Nova::Renderer::OpenGL

#endif // GL_RENDER_PASS_HPP