#ifndef NODE_HPP
#define NODE_HPP

#include <string>
#include <vector>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Nova {
    namespace Scene {

        struct Node {
            std::string m_Name;

            glm::vec3 m_Position = {0.0f, 0.0f, 0.0f};
            glm::vec3 m_Rotation = {0.0f, 0.0f, 0.0f};
            glm::vec3 m_Scale = {1.0f, 1.0f, 1.0f};

            std::vector<Node*> m_Children;
            Node* m_Parent;

            Node(const std::string& name) : m_Name(name), m_Parent(nullptr) {}
            virtual ~Node();

            void addChild(Node* child);
            void removeChild(Node* child);
            glm::mat4 getModelMatrix() const;
        };

    } // namespace Scene
} // namespace Nova

#endif // NODE_HPP