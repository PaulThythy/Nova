#ifndef HIERARCHY_PANEL_HPP
#define HIERARCHY_PANEL_HPP

#include "Components/MeshComponent.hpp"
#include "Components/TransformComponent.hpp"
#include "Components/CameraComponent.hpp"
#include "Components/LightComponent.hpp"
#include "Components/TagComponent.hpp"
#include "Scene/Scene.hpp"

namespace Nova::GUI::HierarchyPanel {
    void Render(Nova::Scene& scene);
}

#endif //HIERARCHY_PANEL_HPP