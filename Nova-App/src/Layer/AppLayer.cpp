#include "Layer/AppLayer.h"

namespace Nova::App {

    void AppLayer::OnRender() {
        if (m_ShowDemo) {
            ImGui::ShowDemoWindow(&m_ShowDemo);
        }

        {
            ImGui::Begin("Hello, Nova!");

            ImGui::Text("Time: %.3f s", m_Time);
            ImGui::Checkbox("Show Demo Window", &m_ShowDemo);
            ImGui::Checkbox("Show Another Window", &m_ShowAnotherWindow);

            ImGui::End();
        }

        if (m_ShowAnotherWindow) {
            ImGui::Begin("Another Window", &m_ShowAnotherWindow);
            ImGui::Text("This is another window.");
            if (ImGui::Button("Close Me")) {
                m_ShowAnotherWindow = false;
            }
            ImGui::End();
        }
    }


    void AppLayer::OnEvent() {}

}