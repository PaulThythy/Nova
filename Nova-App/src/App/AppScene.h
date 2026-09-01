#ifndef APPSCENE_H
#define APPSCENE_H

#include <memory>
#include <string>

#include "Math/Camera.h"
#include "Scene/Scene.h"

namespace Nova::App {

    /** Owns the editor/game scene and its main camera entity. */
    class AppScene {
    public:
        explicit AppScene(const std::string& name = "Scene_test");

        void SetupDefaultScene();
        void Clear();

        Nova::Core::Scene::Scene& GetScene() { return m_Scene; }
        const Nova::Core::Scene::Scene& GetScene() const { return m_Scene; }

        Nova::Core::Math::Camera* GetCamera() { return m_Camera.get(); }
        const Nova::Core::Math::Camera* GetCamera() const { return m_Camera.get(); }

        std::string GetName() const { return m_Scene.GetName(); }

    private:
        Nova::Core::Scene::Scene m_Scene{"Scene_test"};
        std::shared_ptr<Nova::Core::Math::Camera> m_Camera;
    };

} // namespace Nova::App

#endif // APPSCENE_H