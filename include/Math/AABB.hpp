#ifndef AABB_H
#define AABB_H

#include <glm/glm.hpp>

namespace Nova::Math {

    struct AABB {
        glm::vec3 m_Min{ std::numeric_limits<float>::max() };
        glm::vec3 m_Max{ -std::numeric_limits<float>::max() };

        void Expand(const glm::vec3& p) {
            m_Min = glm::min(m_Min, p);
            m_Max = glm::max(m_Max, p);
        }
        glm::vec3 Center() const {return 0.5f * (m_Min + m_Max);}
        glm::vec3 HalfExtents() const {return 0.5f * (m_Max - m_Min);}
        bool Valid() const {return (m_Min.x <= m_Max.x && m_Min.y <= m_Max.y && m_Min.z <= m_Max.z);}
    };

    inline AABB ComputeAABB(const std::vector<glm::vec3>& points) {
        AABB b;
        for (const auto& p : points) b.Expand(p);
        return b;
    }
}

#endif //AABB_H