#include "VulkanLayer.h"

#include "Core/Application.h"
#include "Core/GraphicsAPI.h"
#include "Core/Log.h"
#include "Renderer/RHI/RHI_RenderGraphBuilder.h"

#include "RHILayer.h"

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

    namespace VK = Nova::Core::Renderer::Backends::Vulkan;
    namespace RHI = Nova::Core::Renderer::RHI;

    VK::VK_SwapchainDesc swapDesc{};
    swapDesc.m_FramesInFlight = 3;
    swapDesc.m_CreateSurface = true;
    swapDesc.m_EnableSwapchain = true;
    swapDesc.m_PreferredPresentMode = VK::VK_PresentMode::Default;

    m_Renderer = std::make_unique<VK::VK_Renderer>();
    if (!m_Renderer->Create(swapDesc)) {
        NV_LOG_ERROR("VulkanLayer: failed to create Vulkan renderer");
        m_Renderer.reset();
        return;
    }

    int width = 0, height = 0;
    window.GetWindowSize(width, height);
    if (width <= 0 || height <= 0) {
        width = 1280;
        height = 720;
    }

    // Frame-graph declaration stays RHI (API-agnostic); only the backend is Vulkan.
    RHI::RHI_RenderGraphBuilder rg;
    const auto backbuffer = rg.ImportTexture({
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
        RHI::RHI_TextureFormat::RGBA8,
        RHI::RHI_TextureUsage::ColorAttachment,
    }, RHI::RHI_ResourceState::Present);

    // Minimal present pass: clears the swapchain (black) and leaves rendering
    // open so ImGui can draw — same pattern as AppRenderer's "UI" pass.
    rg.AddPass("UI",
        [&](RHI::RHI_PassBuilder& b) {
            b.Write(backbuffer);
            b.PresentOnly();
        },
        [](RHI::IPassContext& /*ctx*/) {});

    m_Renderer->SetRenderGraph(rg.Build(api));
    NV_LOG_INFO("VulkanLayer: Vulkan renderer + present graph ready");
}

void VulkanLayer::OnDetach() {
    NV_LOG_INFO("VulkanLayer detached");

    if (m_Renderer) {
        Nova::Core::Application::Get().GetImGuiLayer().DestroyImGuiBackend(Nova::Core::GraphicsAPI::Vulkan);
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
    ImGui::Text("VK_Renderer: %p", static_cast<void*>(m_Renderer.get()));
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
    if (e.IsRepeat())
        return false;

    if (e.GetKeyCode() == SDLK_SPACE) {
        Nova::Core::Application::Get().GetLayerStack().QueueLayerTransition<RHILayer>(this);
        NV_LOG_INFO("VulkanLayer: transition to RHILayer requested");
        return true;
    }

    return false;
}