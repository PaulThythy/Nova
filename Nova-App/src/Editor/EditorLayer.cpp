#include "Editor/EditorLayer.h"

#include <string>

#include <SDL3/SDL.h>

#include "App/AppLayer.h"
#include "Core/Assert.h"
#include "Core/Log.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/NameComponent.h"
#include "Events/InputEvents.h"
#include "Math/AABB.h"

namespace Nova::App::Editor {

    void EditorLayer::OnAttach() {
        if (g_AppLayer) {
            g_AppLayer->RegisterEditorLayer(this);
            g_AppLayer->ShowGrid(true);
        }
    }

    void EditorLayer::OnDetach() {
        ClearFocus();
        if (g_AppLayer) {
            g_AppLayer->RegisterEditorLayer(nullptr);
            g_AppLayer->ShowGrid(false);
        }
    }

    void EditorLayer::OnUpdate(float) {}

    void EditorLayer::OnBegin() {}

    void EditorLayer::OnRender() {}

    void EditorLayer::OnEnd() {}

    void EditorLayer::OnImGuiRender() {}

    void EditorLayer::OnEvent(Nova::Core::Events::Event& e) {
        Nova::Core::Events::EventDispatcher dispatcher(e);
        dispatcher.Dispatch<Nova::Core::Events::KeyPressedEvent>([this](Nova::Core::Events::KeyPressedEvent& ev) {
            return OnKeyPressed(ev);
        });
    }

    bool EditorLayer::OnKeyPressed(Nova::Core::Events::KeyPressedEvent& e) {
        if (e.IsRepeat())
            return false;

        if (e.GetKeyCode() == SDLK_ESCAPE) {
            ClearFocus();
            return true;
        }
        return false;
    }

    void EditorLayer::PickAtViewportUV(float u, float v, bool addToSelection) {
        if (!g_AppLayer)
            return;
        auto* camera = g_AppLayer->GetCamera();
        if (!camera)
            return;
        m_Selection.PickAtViewportUV(g_AppLayer->GetScene(), *camera, u, v, addToSelection);
    }

    void EditorLayer::FocusAtViewportUV(float u, float v) {
        if (!g_AppLayer)
            return;
        auto* camera = g_AppLayer->GetCamera();
        if (!camera)
            return;

        auto& scene = g_AppLayer->GetScene();

        Nova::Core::Math::AABB worldAabb{};
        const entt::entity entity = EditorSelection::PickEntityAtViewportUV(
            scene, *camera, u, v, &worldAabb);
        if (entity == entt::null)
            return;

        if (!EditorSelection::ComputeEntityWorldAABB(scene, entity, worldAabb))
            return;

        std::string name = "unnamed";
        if (auto* nc = scene.GetRegistry().try_get<Nova::Core::ECS::Components::NameComponent>(entity))
            name = nc->m_Name;

        uint32_t triangleCount = 0;
        if (auto* mc = scene.GetRegistry().try_get<Nova::Core::ECS::Components::MeshComponent>(entity)) {
            if (mc->m_MeshAsset && mc->m_MeshAsset->IsLoaded()) {
                if (auto cpuMesh = mc->m_MeshAsset->GetCPUMesh())
                    triangleCount = static_cast<uint32_t>(cpuMesh->GetIndices().size() / 3);
            }
        }

        const glm::vec3 center = worldAabb.GetCenter();
        const glm::vec3 extents = worldAabb.GetExtents();

        m_Selection.SetFocused(entity, center, extents, name, triangleCount);
        g_AppLayer->BeginOrbitFocus(center, extents);
    }

    void EditorLayer::ClearFocus() {
        m_Selection.Clear();
        if (g_AppLayer)
            g_AppLayer->ExitOrbitMode();
    }

} // namespace Nova::App::Editor