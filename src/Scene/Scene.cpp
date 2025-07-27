#include "Scene/Scene.hpp"

namespace Nova {
    namespace Scene {

        Scene::~Scene() {
            for (Nova::Scene::Node* node : m_Roots) {
                delete node;
            }
        }

        void Scene::addNode(Nova::Scene::Node* node) {
            if (node && std::find(m_Roots.begin(), m_Roots.end(), node) == m_Roots.end())
                m_Roots.push_back(node);
        }

        void Scene::removeRoot(Nova::Scene::Node* node) {
            m_Roots.erase(std::remove(m_Roots.begin(), m_Roots.end(), node), m_Roots.end());
        }

        Nova::Scene::Node* Scene::findRootByName(const std::string& name) const {
            for (Nova::Scene::Node* node : m_Roots) {
                if (node->m_Name == name) return node;
            }
            return nullptr;
        }

    } // namespace Scene
} // namespace Nova