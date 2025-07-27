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

    } // namespace Scene
} // namespace Nova