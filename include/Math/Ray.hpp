#ifndef RAY_HPP
#define RAY_HPP

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/norm.hpp>

namespace Nova::Math {
    struct Ray {
        glm::vec3 m_Origin {0.0f};
        glm::vec3 m_Direction {0.0f, 0.0f, 1.0f};

        Ray() = default;
        Ray(const glm::vec3& origin, const glm::vec3& direction) : m_Origin(origin), m_Direction(glm::normalize(direction)) {}
        ~Ray() = default;

        glm::vec3 at(float t) const { return m_Origin + t * m_Direction; }
    };

    static inline bool rayTriangle( const Ray& ray,
                                    const glm::vec3& v0,
                                    const glm::vec3& v1,
                                    const glm::vec3& v2,
                                    float& outT,
                                    float eps = 1e-6f) {
        const glm::vec3 e1 = v1 - v0;
        const glm::vec3 e2 = v2 - v0;
        const glm::vec3 p  = glm::cross(ray.m_Direction, e2);
        const float det = glm::dot(e1, p);
        if (det > -eps && det < eps) return false;
        const float invDet = 1.0f / det;

        const glm::vec3 tvec = ray.m_Origin - v0;
        const float u = glm::dot(tvec, p) * invDet;
        if (u < 0.0f || u > 1.0f) return false;

        const glm::vec3 q = glm::cross(tvec, e1);
        const float v = glm::dot(ray.m_Direction, q) * invDet;
        if (v < 0.0f || u + v > 1.0f) return false;

        const float t = glm::dot(e2, q) * invDet;
        if (t < 0.0f) return false;
        outT = t;
        return true;
    }


    inline bool rayAABB(const Ray& ray,
                        const glm::vec3& bmin,
                        const glm::vec3& bmax,
                        float& outTmin, float& outTmax)
    {
        glm::vec3 invD = 1.0f / ray.m_Direction;
        glm::vec3 t0 = (bmin - ray.m_Origin) * invD;
        glm::vec3 t1 = (bmax - ray.m_Origin) * invD;

        glm::vec3 tmin3 = glm::min(t0, t1);
        glm::vec3 tmax3 = glm::max(t0, t1);

        float tmin = glm::compMax(tmin3);
        float tmax = glm::compMin(tmax3);

        if (tmax < 0.0f || tmin > tmax) return false;
        outTmin = tmin;
        outTmax = tmax;
        return true;
    }

    inline bool rayOBB(const Ray& ray,
                       const glm::mat4& model,          // OBB transform (TRS)
                       const glm::vec3& localMin,       // AABB local min
                       const glm::vec3& localMax,       // AABB local max
                       float& outTmin, float& outTmax)
    {
        glm::mat4 inv = glm::inverse(model);
        glm::vec3 rO = glm::vec3(inv * glm::vec4(ray.m_Origin, 1.0f));
        glm::vec3 rD = glm::normalize(glm::vec3(inv * glm::vec4(ray.m_Direction, 0.0f)));
        return rayAABB(Ray{rO, rD}, localMin, localMax, outTmin, outTmax);
    }

    inline glm::vec2 screenToNDC(const glm::vec2& mousePos,
                                 const glm::vec2& viewportSize,
                                 bool originTopLeft = true)
    {
        glm::vec2 ndc;
        ndc.x = ( 2.0f * mousePos.x) / viewportSize.x - 1.0f;
        ndc.y = ( 2.0f * mousePos.y) / viewportSize.y;
        ndc.y = originTopLeft ? (1.0f - 2.0f * ndc.y) : (2.0f * ndc.y - 1.0f);
        return ndc;
    }

    inline Ray mouseRayFromViewport(const glm::vec2& mousePos,
                                    const glm::vec2& viewportSize,
                                    const glm::mat4& view,
                                    const glm::mat4& proj,
                                    const glm::vec3& camWorldPos,
                                    bool originTopLeft = true)
    {
        glm::vec2 ndc = screenToNDC(mousePos, viewportSize, originTopLeft);
        glm::vec4 clip{ ndc.x, ndc.y, -1.0f, 1.0f };

        glm::mat4 invVP = glm::inverse(proj * view);
        glm::vec4 world = invVP * clip;
        world /= world.w;

        glm::vec3 dir = glm::normalize(glm::vec3(world) - camWorldPos);
        return Ray{ camWorldPos, dir };
    }

    inline Ray mouseClickRayCast(const glm::vec2& mousePos,
                                 const glm::vec2& viewportSize,
                                 const glm::mat4& view,
                                 const glm::mat4& proj,
                                 const glm::vec3& camWorldPos)
    {
        return mouseRayFromViewport(mousePos, viewportSize, view, proj, camWorldPos, /*originTopLeft=*/true);
    }
}

#endif // RAY_HPP