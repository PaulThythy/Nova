#ifndef INSPECTOR_PANEL_HPP
#define INSPECTOR_PANEL_HPP

#include "Scene/Scene.hpp"

namespace Nova::GUI::InspectorPanel {
    void Render(Nova::Scene& scene);

    void DrawTRSvectors(const char* label, const char* xButtonLabel, const char* xInputLabel,
        const char* yButtonLabel, const char* yInputLabel,
        const char* zButtonLabel, const char* zInputLabel,
        glm::vec3& values, float inputW, float badgeW, float groupSpacing);

    void DrawTransform(entt::registry& reg, entt::entity e, float inputW, float badgeW, float groupSpacing);

    void DrawMeshRenderer(entt::registry& reg, entt::entity e);

    void DrawLightComponent(entt::registry& reg, entt::entity e);
}

#endif //INSPECTOR_PANEL_HPP