#pragma once

#include "core/src/pch.h"

void check_error_bool(const bool res)
{
    if (!res) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", SDL_GetError());
    }
}

template<typename T>
T* check_error_ptr(T* ptr) {
    if (!ptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", SDL_GetError());
    }
    return ptr;
}


glm::vec3 JPHVec3ToGLM(JPH::Vec3Arg vec) {

    return glm::vec3(vec.GetX(), vec.GetY(), vec.GetZ());
}

JPH::Vec3 GLMVec3ToJPH(const glm::vec3 vec) {

    return JPH::Vec3(vec.x, vec.y, vec.z);
}


glm::quat JPHQuatToGLM(JPH::QuatArg quat) {

    return glm::quat(quat.GetW(), quat.GetX(), quat.GetY(), quat.GetZ()); // w, x, y, z order      
}

JPH::Quat GLMQuatToJPH(const glm::quat quat) {

    return JPH::Quat(quat.x, quat.y, quat.z, quat.w); // x, y, z, w order
}