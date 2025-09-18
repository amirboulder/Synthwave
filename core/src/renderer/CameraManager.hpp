#pragma once

#include "renderer.hpp"

#include "../common.hpp"

class CameraManager {

public:

    Camera playerCam;
    Camera freeCam;


    CameraManager(RendererConfig& renderConfig)
        : playerCam(renderConfig, glm::vec3(3.62839f, -13.5578f, -26.0563f)), freeCam(renderConfig, glm::vec3(3.62839f, -13.5578f, -26.0563f))
    {

    }
};