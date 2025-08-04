#include "GUI/Panels/ViewportPanel.hpp"
#include "Renderer/Renderer.hpp"
#include "imgui.h"

namespace Nova::GUI {

    void renderViewportPanel(Nova::Renderer::IRenderer* renderer) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        ImVec2 size = ImGui::GetContentRegionAvail();
        renderer->updateViewportSize((int)size.x, (int)size.y);

        ImGui::Image((ImTextureID)renderer->getImGuiTextureID(), size);

        ImGui::End();
        ImGui::PopStyleVar();
    }

} // namespace Nova::GUI