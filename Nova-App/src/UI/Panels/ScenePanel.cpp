#include "UI/Panels/ScenePanel.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

#include "imgui.h"
#include "imgui_internal.h"
#include "ImGuizmo.h"

#include "App/AppLayer.h"
#include "Core/Application.h"
#include "ECS/Components/TransformComponent.h"
#include "Editor/EditorLayer.h"
#include "Events/ApplicationEvents.h"

namespace Nova::App::UI::Panels::ScenePanel {

    using Nova::App::Editor::EditorLayer;
    using Nova::App::Editor::GizmoOperation;
    using Nova::App::Editor::GizmoSpace;
    using Nova::Core::ECS::Components::TransformComponent;

    static ImGuizmo::OPERATION ToImGuizmoOperation(GizmoOperation operation) {
        switch (operation) {
            case GizmoOperation::Rotate: return ImGuizmo::ROTATE;
            case GizmoOperation::Scale: return ImGuizmo::SCALE;
            case GizmoOperation::Translate:
            default: return ImGuizmo::TRANSLATE;
        }
    }

    static void DrawGizmoModeButton(EditorLayer* editor, const char* label, GizmoOperation operation) {
        const bool selected = editor && editor->GetGizmoOperation() == operation;
        if (selected)
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

        if (ImGui::Button(label))
            editor->SetGizmoOperation(operation);

        if (selected)
            ImGui::PopStyleColor();
    }

    static void DrawGizmoSpaceButton(EditorLayer* editor, const char* label, GizmoSpace space) {
        const bool selected = editor && editor->GetGizmoSpace() == space;
        if (selected)
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

        if (ImGui::Button(label))
            editor->SetGizmoSpace(space);

        if (selected)
            ImGui::PopStyleColor();
    }

    static ImGuizmo::MODE ToImGuizmoMode(EditorLayer* editor) {
        // ImGuizmo scale is local-only; keep visual/world toggle but force LOCAL for scale.
        if (editor->GetGizmoOperation() == GizmoOperation::Scale)
            return ImGuizmo::LOCAL;
        return editor->GetGizmoSpace() == GizmoSpace::World ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
    }

    static void DrawSceneToolbarBar() {
        // A header-like bar INSIDE the scene window, just under the title.
        const float barH = 34.0f;

        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse;

        ImGui::BeginChild("##SceneToolbarBar", ImVec2(0.0f, barH), true, flags);

        AppLayer* app = Nova::App::g_AppLayer;
        const bool playing = app && (app->GetSceneState() == AppLayer::SceneState::Play);

        const ImVec2 iconSize(18.0f, 18.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 4.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.10f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 1.0f, 1.0f, 0.20f));

        // Left side: Play/Stop
        if (!playing) {
            void* playIcon = app ? app->GetPlayIconImGuiID() : nullptr;
            const bool clicked = playIcon
                ? ImGui::ImageButton("##Play", playIcon, iconSize)
                : ImGui::Button("Play");
            if (clicked) {
                if (Nova::App::g_AppLayer)
                    Nova::App::g_AppLayer->RequestPlay();
            }
        }
        else {
            void* pauseIcon = app ? app->GetPauseIconImGuiID() : nullptr;
            const bool clicked = pauseIcon
                ? ImGui::ImageButton("##Pause", pauseIcon, iconSize)
                : ImGui::Button("Pause");
            if (clicked) {
                if (Nova::App::g_AppLayer)
                    Nova::App::g_AppLayer->RequestStop();
            }
        }

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();

        ImGui::EndChild();
    }

    static void DecomposeMatrixToTRS(
        const glm::mat4& matrix,
        glm::vec3& outTranslation,
        glm::quat& outRotation,
        glm::vec3& outScale)
    {
        outTranslation = glm::vec3(matrix[3]);

        glm::vec3 column0 = glm::vec3(matrix[0]);
        glm::vec3 column1 = glm::vec3(matrix[1]);
        glm::vec3 column2 = glm::vec3(matrix[2]);

        outScale = glm::vec3(
            glm::length(column0),
            glm::length(column1),
            glm::length(column2));

        if (glm::determinant(glm::mat3(matrix)) < 0.0f)
            outScale.x = -outScale.x;

        if (std::abs(outScale.x) > 1e-8f) column0 /= outScale.x;
        if (std::abs(outScale.y) > 1e-8f) column1 /= outScale.y;
        if (std::abs(outScale.z) > 1e-8f) column2 /= outScale.z;

        outRotation = glm::normalize(glm::quat_cast(glm::mat3(column0, column1, column2)));
    }

    static void WriteTransformFromQuatTRS(
        TransformComponent& transform,
        const glm::vec3& translation,
        const glm::quat& rotation,
        const glm::vec3& scale)
    {
        transform.m_Translation = translation;
        // Component keeps Euler; convert only when writing back.
        transform.m_Rotation = glm::eulerAngles(rotation);
        transform.m_Scale = scale;
    }

    static bool DrawViewCube(const ImVec2& viewportMin, const ImVec2& viewportMax) {
        if (!g_AppLayer)
            return false;

        auto* camera = g_AppLayer->GetCamera();
        if (!camera)
            return false;

        constexpr float kCubeSize = 100.0f;
        constexpr float kPad = 10.0f;
        const ImVec2 cubePos(viewportMax.x - kCubeSize - kPad, viewportMin.y + kPad);
        const ImVec2 cubeSize(kCubeSize, kCubeSize);
        const ImVec2 viewportSize(viewportMax.x - viewportMin.x, viewportMax.y - viewportMin.y);

        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(viewportMin.x, viewportMin.y, viewportSize.x, viewportSize.y);

        glm::mat4 view = camera->GetViewMatrix();
        const float distance = std::max(glm::length(camera->m_LookFrom - camera->m_LookAt), 0.2f);

        ImGuizmo::ViewManipulate(
            glm::value_ptr(view),
            distance,
            cubePos,
            cubeSize,
            0x10101010);

        const bool usingViewCube =
            ImGuizmo::IsUsingViewManipulate() || ImGuizmo::IsViewManipulateHovered();

        if (ImGuizmo::IsUsingViewManipulate()) {
            const glm::mat4 invView = glm::inverse(view);
            const glm::vec3 eye = glm::vec3(invView[3]);
            const glm::vec3 forward = -glm::normalize(glm::vec3(invView[2]));
            const glm::vec3 up = glm::normalize(glm::vec3(invView[1]));

            camera->m_LookFrom = eye;
            camera->m_LookAt = eye + forward * distance;
            camera->m_Up = up;
            g_AppLayer->SyncCameraControllerFromCamera();
        }

        return usingViewCube;
    }

    static bool DrawTransformGizmo(EditorLayer* editor, const ImVec2& viewportMin, const ImVec2& viewportSize) {
        if (!editor || !g_AppLayer)
            return false;

        auto* camera = g_AppLayer->GetCamera();
        if (!camera)
            return false;

        const entt::entity selectedEntity = editor->GetSelectedEntity();
        if (selectedEntity == entt::null)
            return false;

        auto& registry = g_AppLayer->GetScene().GetRegistry();
        auto* transform = registry.try_get<TransformComponent>(selectedEntity);
        if (!transform)
            return false;

        struct DragStartPose {
            entt::entity m_Entity{ entt::null };
            glm::vec3 m_Translation{ 0.0f };
            glm::quat m_Rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
            glm::vec3 m_Scale{ 1.0f };
        };
        static bool s_DragActive = false;
        static std::vector<DragStartPose> s_DragStarts;
        // Keep a stable matrix during drag so we never rebuild from Euler mid-gesture.
        static glm::mat4 s_DragMatrix{ 1.0f };

        ImGuizmo::SetOrthographic(!camera->m_IsPerspective);
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(viewportMin.x, viewportMin.y, viewportSize.x, viewportSize.y);

        if (!s_DragActive)
            s_DragMatrix = transform->GetTransform();

        glm::mat4 view = camera->GetViewMatrix();
        glm::mat4 projection = camera->GetProjectionMatrix();

        // ImGuizmo expects OpenGL clip-space (+Y up). Nova's Camera projection
        // applies a Vulkan Y-flip (proj[1][1] *= -1); undo it for the gizmo only.
        projection[1][1] *= -1.0f;

        const bool changed = ImGuizmo::Manipulate(
            glm::value_ptr(view),
            glm::value_ptr(projection),
            ToImGuizmoOperation(editor->GetGizmoOperation()),
            ToImGuizmoMode(editor),
            glm::value_ptr(s_DragMatrix));

        const bool isUsing = ImGuizmo::IsUsing();
        editor->SetGizmoDragging(isUsing);

        if (isUsing && !s_DragActive) {
            s_DragActive = true;
            s_DragStarts.clear();
            for (entt::entity entity : editor->GetSelectedEntities()) {
                auto* tc = registry.try_get<TransformComponent>(entity);
                if (!tc)
                    continue;
                s_DragStarts.push_back({
                    entity,
                    tc->m_Translation,
                    glm::normalize(glm::quat(tc->m_Rotation)),
                    tc->m_Scale
                });
            }
        }

        if (changed) {
            glm::vec3 translation{};
            glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
            glm::vec3 scale{ 1.0f };
            DecomposeMatrixToTRS(s_DragMatrix, translation, rotation, scale);
            WriteTransformFromQuatTRS(*transform, translation, rotation, scale);

            // Propagate the same TRS delta from the primary entity to the rest of the selection.
            if (s_DragActive && s_DragStarts.size() > 1) {
                const DragStartPose* primaryStart = nullptr;
                for (const DragStartPose& start : s_DragStarts) {
                    if (start.m_Entity == selectedEntity) {
                        primaryStart = &start;
                        break;
                    }
                }

                if (primaryStart) {
                    const glm::vec3 deltaTranslation = translation - primaryStart->m_Translation;
                    const glm::quat deltaRotation = rotation * glm::inverse(primaryStart->m_Rotation);
                    const glm::vec3 scaleRatio{
                        scale.x / std::max(std::abs(primaryStart->m_Scale.x), 1e-6f),
                        scale.y / std::max(std::abs(primaryStart->m_Scale.y), 1e-6f),
                        scale.z / std::max(std::abs(primaryStart->m_Scale.z), 1e-6f),
                    };

                    for (const DragStartPose& start : s_DragStarts) {
                        if (start.m_Entity == selectedEntity)
                            continue;

                        auto* tc = registry.try_get<TransformComponent>(start.m_Entity);
                        if (!tc)
                            continue;

                        const glm::quat otherRotation = glm::normalize(deltaRotation * start.m_Rotation);
                        WriteTransformFromQuatTRS(
                            *tc,
                            start.m_Translation + deltaTranslation,
                            otherRotation,
                            start.m_Scale * scaleRatio);
                    }
                }
            }
        }

        if (!isUsing) {
            s_DragActive = false;
            s_DragStarts.clear();
        }

        // Block picking while hovering or dragging the gizmo.
        return isUsing || ImGuizmo::IsOver();
    }

    static void DrawViewportSettingsOverlay() {
        AppLayer* app = Nova::App::g_AppLayer;
        if (!app)
            return;

        // Overlay controls in the top-left corner of the rendered viewport.
        ImGui::SetCursorPos(ImVec2(8.0f, 8.0f));

        static const char* kModeNames[] = {
            "Lit",
            "Unlit",
            "Wireframe",
            "Normals",
            "Positions",
            "Vertex Color",
            "Depth",
        };

        int mode = static_cast<int>(app->GetRenderDebugMode());
        ImGui::SetNextItemWidth(150.0f);
        if (ImGui::Combo("##RenderMode", &mode, kModeNames, static_cast<int>(RenderDebugMode::Count))) {
            auto debugMode = static_cast<RenderDebugMode>(mode);
            app->SetRenderDebugMode(debugMode);
            if (debugMode == RenderDebugMode::Depth)
                app->ShowGrid(false);
        }

        ImGui::SameLine();

        bool showAABB = app->IsAABBVisible();
        if (ImGui::Checkbox("Show AABB", &showAABB))
            app->ShowAABB(showAABB);

        ImGui::SameLine();

        const bool depthMode = (app->GetRenderDebugMode() == RenderDebugMode::Depth);
        bool showGrid = app->IsGridVisible();
        ImGui::BeginDisabled(depthMode);
        if (ImGui::Checkbox("Show Grid", &showGrid))
            app->ShowGrid(showGrid);
        ImGui::EndDisabled();

        EditorLayer* editor = app->GetEditorLayer();
        if (editor) {
            ImGui::SameLine();
            DrawGizmoModeButton(editor, "T", GizmoOperation::Translate);
            ImGui::SameLine();
            DrawGizmoModeButton(editor, "R", GizmoOperation::Rotate);
            ImGui::SameLine();
            DrawGizmoModeButton(editor, "S", GizmoOperation::Scale);

            ImGui::SameLine();
            const bool scaleForcesLocal = editor->GetGizmoOperation() == GizmoOperation::Scale;
            ImGui::BeginDisabled(scaleForcesLocal);
            DrawGizmoSpaceButton(editor, "Local", GizmoSpace::Local);
            ImGui::SameLine();
            DrawGizmoSpaceButton(editor, "World", GizmoSpace::World);
            ImGui::EndDisabled();
        }

        // Frame stats in the top-right corner of the viewport.
        const float dt = app->GetDeltaTime();
        const float fps = (dt > 0.0f) ? (1.0f / dt) : 0.0f;
        const float frameMs = dt * 1000.0f;

        char fpsText[32];
        char msText[32];
        std::snprintf(fpsText, sizeof(fpsText), "%.1f FPS", fps);
        std::snprintf(msText, sizeof(msText), "%.2f ms", frameMs);

        const float pad = 8.0f;
        const float textW = (std::max)(ImGui::CalcTextSize(fpsText).x, ImGui::CalcTextSize(msText).x);
        const ImVec2 contentMax = ImGui::GetWindowContentRegionMax();

        ImGui::SetCursorPos(ImVec2(contentMax.x - textW - pad, pad));
        ImGui::TextUnformatted(fpsText);
        ImGui::SetCursorPosX(contentMax.x - ImGui::CalcTextSize(msText).x - pad);
        ImGui::TextUnformatted(msText);

        if (editor && editor->HasFocus()) {
            const Editor::FocusInfo& focusInfo = editor->GetFocusInfo();

            char focusName[128];
            std::snprintf(focusName, sizeof(focusName), "Focus: %s", focusInfo.m_Name.c_str());

            char centerText[128];
            std::snprintf(centerText, sizeof(centerText), "Center: %.2f, %.2f, %.2f",
                focusInfo.m_AabbCenter.x,
                focusInfo.m_AabbCenter.y,
                focusInfo.m_AabbCenter.z);

            char sizeText[128];
            std::snprintf(sizeText, sizeof(sizeText), "Size: %.2f, %.2f, %.2f",
                focusInfo.m_AabbExtents.x * 2.0f,
                focusInfo.m_AabbExtents.y * 2.0f,
                focusInfo.m_AabbExtents.z * 2.0f);

            char triangleText[64];
            std::snprintf(triangleText, sizeof(triangleText), "Triangles: %u", focusInfo.m_TriangleCount);

            const float focusW = (std::max)({
                ImGui::CalcTextSize(focusName).x,
                ImGui::CalcTextSize(centerText).x,
                ImGui::CalcTextSize(sizeText).x,
                ImGui::CalcTextSize(triangleText).x,
            });

            char escapeText[128];
            std::snprintf(escapeText, sizeof(escapeText), "Press ESC to quit focus mode");

            const float lineH = ImGui::GetTextLineHeightWithSpacing();
            float y = pad + lineH * 2.0f;

            ImGui::SetCursorPos(ImVec2(contentMax.x - focusW - pad, y));
            ImGui::TextUnformatted(focusName);
            y += lineH;

            ImGui::SetCursorPos(ImVec2(contentMax.x - ImGui::CalcTextSize(centerText).x - pad, y));
            ImGui::TextUnformatted(centerText);
            y += lineH;

            ImGui::SetCursorPos(ImVec2(contentMax.x - ImGui::CalcTextSize(sizeText).x - pad, y));
            ImGui::TextUnformatted(sizeText);
            y += lineH;

            ImGui::SetCursorPos(ImVec2(contentMax.x - ImGui::CalcTextSize(triangleText).x - pad, y));
            ImGui::TextUnformatted(triangleText);
            y += lineH;

            ImGui::SetCursorPos(ImVec2(contentMax.x - ImGui::CalcTextSize(escapeText).x - pad, y));
            ImGui::TextUnformatted(escapeText);
        }
    }

    void Render(const std::string& sceneName) {
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        ImGui::Begin(sceneName.c_str(), nullptr, flags);
        EditorLayer* editor = Nova::App::g_AppLayer ? Nova::App::g_AppLayer->GetEditorLayer() : nullptr;

        // 1) Toolbar INSIDE the scene panel (requested)
        DrawSceneToolbarBar();

        // 2) Viewport (framebuffer) with settings overlaid on top
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::BeginChild("##Viewport", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        // Expose hover state to AppLayer so orbit-camera input is restricted to this area.
        if (Nova::App::g_AppLayer)
            Nova::App::g_AppLayer->SetViewportHovered(ImGui::IsWindowHovered());

        ImVec2 size = ImGui::GetContentRegionAvail();

        static ImVec2 s_LastSize(-1.0f, -1.0f);
        if (size.x > 0.0f && size.y > 0.0f &&
            (size.x != s_LastSize.x || size.y != s_LastSize.y)) {

            s_LastSize = size;

            // Keep the same resize pipeline name: "Viewport"
            using namespace Nova::Core::Events;
            ImGuiPanelResizeEvent e("Viewport", size.x, size.y);
            Nova::Core::Application::Get().OnEvent(e);
        }

        if (Nova::App::g_AppLayer->GetRenderer()) {
            if (void* textureId = Nova::App::g_AppLayer->GetRenderer()->GetTextureImGuiID(Nova::App::g_AppLayer->GetSceneColor())) {
                ImGui::Image(textureId, size, ImVec2(0, 0), ImVec2(1, 1));
                const ImVec2 min = ImGui::GetItemRectMin();
                const ImVec2 max = ImGui::GetItemRectMax();
                const ImVec2 viewportSize(max.x - min.x, max.y - min.y);
                const bool viewportHovered = ImGui::IsItemHovered();
                const bool gizmoActive = DrawTransformGizmo(editor, min, viewportSize);
                const bool viewCubeActive = DrawViewCube(min, max);

                // Draw overlay before pick so IsAnyItemHovered/Active covers its controls.
                DrawViewportSettingsOverlay();

                // Left-click: select. Double-click: focus (orbit camera on object AABB).
                // Selection is editor-only (EditorLayer must be active).
                // Skip pick when overlay widgets (or gizmo) own the click, otherwise a miss
                // clears the selection.
                if (viewportHovered) {
                    const ImVec2 mouse = ImGui::GetMousePos();
                    const float w = viewportSize.x;
                    const float h = viewportSize.y;
                    if (w > 1e-3f && h > 1e-3f) {
                        const float u = (mouse.x - min.x) / w;
                        const float v = (mouse.y - min.y) / h;
                        Nova::App::g_AppLayer->SetViewportCursorUV(u, v);

                        if (editor) {
                            const bool overlayBusy = ImGui::IsAnyItemHovered() || ImGui::IsAnyItemActive();
                            const bool blockPick = gizmoActive || viewCubeActive || overlayBusy || editor->ShouldBlockViewportPick();
                            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && !blockPick) {
                                editor->FocusAtViewportUV(u, v);
                            } else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                                if (blockPick) {
                                    editor->ConsumeViewportPickBlock();
                                } else if (!editor->HasFocus()) {
                                    const bool addToSelection = ImGui::IsKeyDown(ImGuiKey_LeftShift);
                                    editor->PickAtViewportUV(u, v, addToSelection);
                                }
                            } else if (editor->ShouldBlockViewportPick() && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                                editor->ConsumeViewportPickBlock();
                            }
                        }
                    }
                }
            }
        }
        else {
            ImGui::TextUnformatted("Framebuffer not ready.");
            DrawViewportSettingsOverlay();
        }

        ImGui::EndChild();
        ImGui::PopStyleVar();

        ImGui::End();
    }

} // namespace Nova::App::UI::Panels::ScenePanel