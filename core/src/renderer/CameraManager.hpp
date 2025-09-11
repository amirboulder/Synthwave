#pragma once

#include "renderer.hpp"

#include "../common.hpp"

class CameraManager {

public:
    Camera playerCam;
    Camera freeCam;
    Renderer* renderer;

    CameraManager(RendererConfig renderConfig)
        :playerCam(renderConfig), freeCam(renderConfig)
    {

    }

    void switchContext(AppContext context) {
        switch (context) {

        case AppContext::player:
            renderer->activeCamera = &playerCam;
            break;

        case AppContext::freeCam:
            renderer->activeCamera = &freeCam;
            break;
        }
    }
};