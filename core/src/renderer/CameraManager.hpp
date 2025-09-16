#pragma once

#include "renderer.hpp"

#include "../common.hpp"

class CameraManager {

public:

    Camera playerCam;
    Camera freeCam;

    Renderer & renderer;

    glm::vec3 defaultCamPos = glm::vec3(3.62839f, -13.5578f, -26.0563f);

    CameraManager(Renderer& renderer,RendererConfig & renderConfig, AppContext context)
        :renderer(renderer),playerCam(renderConfig, glm::vec3(3.62839f, -13.5578f, -26.0563f)), freeCam(renderConfig, glm::vec3(3.62839f, -13.5578f, -26.0563f))
    {
        setContext(context);
    }

    void setContext(AppContext context) {
        switch (context) {

        case AppContext::player:
            renderer.activeCamera = &playerCam;
            break;

        case AppContext::freeCam:
            renderer.activeCamera = &freeCam;
            break;
        }
    }
};