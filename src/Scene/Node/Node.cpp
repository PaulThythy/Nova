#include "Scene/Node/Node.hpp"

namespace Nova {
    namespace Scene {   

        Node::~Node() {
            for (Node* child : m_Children) {
                delete child;
            }
        }

        void Node::addChild(Node* child) {
            if(!child) return;
            child->m_Parent = this;
            m_Children.push_back(child);
        }

        void Node::removeChild(Node* child) {
            if(!child) return;
            auto it = std::find(m_Children.begin(), m_Children.end(), child);
            if (it != m_Children.end()) {
                m_Children.erase(it);
                (*it)->m_Parent = nullptr;
            }
        }

        glm::mat4 Node::getModelMatrix() const {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, m_Position);
            model = glm::rotate(model, glm::radians(m_Rotation.x), glm::vec3(1, 0, 0));
            model = glm::rotate(model, glm::radians(m_Rotation.y), glm::vec3(0, 1, 0));
            model = glm::rotate(model, glm::radians(m_Rotation.z), glm::vec3(0, 0, 1));
            model = glm::scale(model, m_Scale);
            return model;
        }

    } // namespace Scene
} // namespace Nova