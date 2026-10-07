#include "VulkanLayer.h"

#include "Core/Application.h"
#include "Core/GraphicsAPI.h"
#include "Core/Log.h"
#include "Renderer/RHI/RHI_RenderGraphBuilder.h"

#include "imgui.h"

VulkanLayer::VulkanLayer() : Layer("VulkanLayer") {}

void VulkanLayer::OnAttach() {
    NV_LOG_INFO("VulkanLayer attached");

    auto& app = Nova::Core::Application::Get();
    auto& window = app.GetWindow();
    auto& imgui = app.GetImGuiLayer();

    const auto api = Nova::Core::GraphicsAPI::Vulkan;

    // Tear down previous ImGui renderer backend (e.g. SDLRenderer) before
    // destroying window-owned SDL resources and creating Vulkan.
    if (window.GetGraphicsAPI() == Nova::Core::GraphicsAPI::SDLRenderer)
        imgui.DestroyImGuiBackend(Nova::Core::GraphicsAPI::SDLRenderer);

    if (window.GetGraphicsAPI() != api) {
        if (!window.SetGraphicsAPI(api)) {
            NV_LOG_ERROR("VulkanLayer: failed to set GraphicsAPI::Vulkan");
            return;
        }
    }

    // Mark ImGui for Vulkan; InitPresentationResources completes it via SetVulkanInitInfo.
    imgui.SetImGuiBackend(api);

    namespace RG = Nova::Core::Renderer::RHI;

    RG::RHI_SwapchainDesc swapDesc{};
    swapDesc.m_FramesInFlight = 3;
    swapDesc.m_CreateSurface = true;
    swapDesc.m_EnableSwapchain = true;
    swapDesc.m_PreferredPresentMode = RG::RHI_PresentMode::Default;

    m_Renderer = RG::IRenderer::Create(api, swapDesc);
    if (!m_Renderer) {
        NV_LOG_ERROR("VulkanLayer: failed to create Vulkan renderer");
        return;
    }

    int width = 0, height = 0;
    window.GetWindowSize(width, height);
    if (width <= 0 || height <= 0) {
        width = 1280;
        height = 720;
    }

    RG::RHI_RenderGraphBuilder fg;
    const auto backbuffer = fg.ImportTexture({
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
        RG::RHI_TextureFormat::RGBA8,
        RG::RHI_TextureUsage::ColorAttachment,
    }, RG::RHI_ResourceState::Present);

    // Minimal present pass: clears the swapchain (black) and leaves rendering
    // open so ImGui can draw — same pattern as AppRenderer's "UI" pass.
    fg.AddPass("UI",
        [&](RG::RHI_PassBuilder& b) {
            b.Write(backbuffer);
            b.PresentOnly();
        },
        [](RG::IPassContext& /*ctx*/) {});

    m_Renderer->SetRenderGraph(fg.Build(api));
    NV_LOG_INFO("VulkanLayer: Vulkan renderer + present graph ready");
}

void VulkanLayer::OnDetach() {
    NV_LOG_INFO("VulkanLayer detached");

    if (m_Renderer) {
        Nova::Core::Application::Get().GetImGuiLayer().DestroyImGuiBackend(
            Nova::Core::GraphicsAPI::Vulkan);
        m_Renderer->Destroy();
        m_Renderer.reset();
    }
}

void VulkanLayer::OnUpdate(float /*dt*/) {
    if (m_Renderer)
        m_Renderer->Update(0.0f);
}

void VulkanLayer::OnBegin() {
    if (m_Renderer)
        m_Renderer->BeginFrame();
}

void VulkanLayer::OnRender() {
    if (m_Renderer)
        m_Renderer->RenderFrame();
}

void VulkanLayer::OnEnd() {
    if (m_Renderer)
        m_Renderer->EndFrame();
}

void VulkanLayer::OnImGuiRender() {
    ImGui::Begin("VulkanLayer");
    ImGui::Text("Graphics API: Vulkan");
    ImGui::Text("IRenderer: %p", static_cast<void*>(m_Renderer.get()));
    ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Swapchain clear + ImGui via Vulkan");
    ImGui::End();
}

void VulkanLayer::OnEvent(Nova::Core::Events::Event& e) {
    Nova::Core::Events::EventDispatcher dispatcher(e);
    dispatcher.Dispatch<Nova::Core::Events::KeyPressedEvent>([this](Nova::Core::Events::KeyPressedEvent& ev) {
        return OnKeyPressed(ev);
    });
}

bool VulkanLayer::OnKeyPressed(Nova::Core::Events::KeyPressedEvent& e) {
    (void)e;
    return false;
}