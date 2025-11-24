#pragma once

#include "core/src/pch.h"



//both serialization and deserialization
namespace serialization {

	

}

namespace serde{

    std::optional<glm::vec3> deserializeGLMVec3(const rapidjson::Value& obj) {
        if (!obj.IsObject()) return std::nullopt;

        if (!obj.HasMember("x") || !obj.HasMember("y") || !obj.HasMember("z"))
            return std::nullopt;

        return glm::vec3(
            obj["x"].GetFloat(),
            obj["y"].GetFloat(),
            obj["z"].GetFloat()
        );
    }

    std::optional<glm::quat> deserializeGLMQuat(const rapidjson::Value& obj) {
        if (!obj.IsObject()) return std::nullopt;

        if (!obj.HasMember("w") || !obj.HasMember("x") ||
            !obj.HasMember("y") || !obj.HasMember("z"))
            return std::nullopt;

        // glm::quat constructor is (w, x, y, z)
        return glm::quat(
            obj["w"].GetFloat(),
            obj["x"].GetFloat(),
            obj["y"].GetFloat(),
            obj["z"].GetFloat()
        );
    }

    std::optional<Transform> deserializeTransform(const rapidjson::Value& obj) {
        if (!obj.IsObject()) return std::nullopt;

        Transform transform;

        // Deserialize position
        if (obj.HasMember("position")) {
            auto pos = deserializeGLMVec3(obj["position"]);
            if (pos) transform.position = *pos;
        }

        // Deserialize rotation
        if (obj.HasMember("rotation")) {
            auto rot = deserializeGLMQuat(obj["rotation"]);
            if (rot) transform.rotation = *rot;
        }

        // Deserialize scale
        if (obj.HasMember("scale")) {
            auto scl = deserializeGLMVec3(obj["scale"]);
            if (scl) transform.scale = *scl;
        }

        return transform;
    }

}