#pragma once

#include "core/src/pch.h"

using json = nlohmann::json;

// conversion functions need to be in the same namespace as the objects
JPH_NAMESPACE_BEGIN

inline void to_json(nlohmann::json& j, const Vec3& v) {
    j = nlohmann::json{ v.GetX(), v.GetY(), v.GetZ() };
}

inline void from_json(const nlohmann::json& j, Vec3& v) {
    if (j.is_array() && j.size() >= 3)
        v = Vec3(j[0].get<float>(), j[1].get<float>(), j[2].get<float>());
}

inline void to_json(nlohmann::json& j, const Quat& q) {
    j = nlohmann::json{ q.GetX(), q.GetY(), q.GetZ(), q.GetW() };
}

inline void from_json(const nlohmann::json& j, Quat& q) {
    if (j.is_array() && j.size() >= 4)
        q = Quat(j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>());
}

JPH_NAMESPACE_END

namespace glm
{

    // glm::vec3 helper
    inline void to_json(json& j, const glm::vec3& v) {
        j = json{ v.x, v.y, v.z };
    }
    inline void from_json(const json& j, glm::vec3& v) {
        if (j.is_array() && j.size() >= 3)
            v = glm::vec3(j[0].get<float>(), j[1].get<float>(), j[2].get<float>());
    }

}
