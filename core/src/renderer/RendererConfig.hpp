#pragma once
#include <iostream>
#include "INIReader.h"
    
    //This class exists because there is a number of render related data that needs to be passed around
    // and I didn't want the pass around the renderer class itself
    class RendererConfig {
    public:
        int windowWidth = 1920;
        int windowHeight = 1080;

        SDL_GPUSampleCount sampleCountMSAA = SDL_GPU_SAMPLECOUNT_8;

        bool RendererPhysics = false;
        bool DrawBoundingBoxPhysics = false;
        bool DrawShapeWireframePhysics = false;

        glm::vec3 FreeCamPos = glm::vec3(-2.0f, -2.0f, 0.0f);
        glm::vec3 FreeCamFront = glm::vec3(-2.0f, -2.0f, 0.0f);
        float FreeCamYaw = 0.0f;
        float FreeCamPitch = 0.0f;
        float freeCamFov = 45.0f;
        float freeCamNearPlane = 0.1f;
        float freeCamFarPlane = 1000.0f;
        float FreeCamMouseSensitivity = 0.1;


        RendererConfig(const std::string& filepath) {

            if (!std::filesystem::exists(filepath)) {
                SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "renderConfig file does not exist. Creating a new one");

                saveRendererConfigINI(filepath);

            }

            loadRendererConfigINI(filepath);

        }

        
        void saveRendererConfigINI(const std::string& filepath) {
            std::ofstream iniFile(filepath);
            if (!iniFile) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,"Failed to create INI file: %s", filepath.c_str());
            }

            iniFile << "[window]\n";
            iniFile << "width=" << windowWidth << "\n";
            iniFile << "height=" << windowHeight << "\n\n";


            iniFile << "[renderer]\n";
            iniFile << "sampleCountMSAA=" << static_cast<int>(sampleCountMSAA) << "\n";
            iniFile << "RendererPhysics=" << (RendererPhysics ? "true" : "false") << "\n";
            iniFile << "DrawBoundingBoxPhysics=" << (DrawBoundingBoxPhysics ? "true" : "false") << "\n";
            iniFile << "DrawShapeWireframePhysics=" << (DrawShapeWireframePhysics ? "true" : "false") << "\n\n";

            iniFile << "[camera]\n";
            iniFile << "FreeCamPos=" << FreeCamPos.x << "," << FreeCamPos.y << "," << FreeCamPos.z << "\n";
            iniFile << "FreeCamFront=" << FreeCamFront.x << "," << FreeCamFront.y << "," << FreeCamFront.z << "\n";
            iniFile << "FreeCamYaw=" << FreeCamYaw << "\n";
            iniFile << "FreeCamPitch=" << FreeCamPitch << "\n";
            iniFile << "freeCamFov=" << freeCamFov << "\n";
            iniFile << "freeCamNearPlane=" << freeCamNearPlane << "\n";
            iniFile << "freeCamFarPlane=" << freeCamFarPlane << "\n";

            iniFile << "FreeCamMouseSensitivity=" << FreeCamMouseSensitivity << "\n";


            iniFile.close();

            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Renderer config saved to: %s", filepath.c_str());

        }

        //load render config values from ini file
        //sets values to default if they are not found within the config file
        void loadRendererConfigINI(const std::string& filepath) {


            INIReader reader(filepath);

            if (reader.ParseError() < 0) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load file: %s", filepath.c_str());
                return ;
            }

            windowWidth = reader.GetInteger("window", "width", windowWidth);
            windowHeight = reader.GetInteger("window", "height", windowHeight);

            sampleCountMSAA = static_cast<SDL_GPUSampleCount>(
                reader.GetInteger("renderer", "sampleCountMSAA", sampleCountMSAA)
                );

            RendererPhysics = reader.GetBoolean("renderer", "RendererPhysics", RendererPhysics);
            DrawBoundingBoxPhysics = reader.GetBoolean("renderer", "DrawBoundingBoxPhysics", DrawBoundingBoxPhysics);
            DrawShapeWireframePhysics = reader.GetBoolean("renderer", "DrawShapeWireframePhysics", DrawShapeWireframePhysics);


            FreeCamPos = parseVec3(
                reader.Get("camera", "FreeCamPos", "-2,-2,0"),
                glm::vec3(-2.0f, -2.0f, 0.0f)
            );

            FreeCamFront = parseVec3(
                reader.Get("camera", "FreeCamFront", "-2,-2,0"),
                glm::vec3(-2.0f, -2.0f, 0.0f)
            );

            FreeCamYaw = static_cast<float>(
                reader.GetReal("camera", "FreeCamYaw", FreeCamYaw)
                );

            FreeCamPitch = static_cast<float>(
                reader.GetReal("camera", "FreeCamPitch", FreeCamPitch)
                );

            freeCamFov = static_cast<float>(
                reader.GetReal("camera", "freeCamFov", freeCamFov)
                );

            freeCamNearPlane = static_cast<float>(
                reader.GetReal("camera", "freeCamNearPlane", freeCamNearPlane)
                );

            freeCamFarPlane = static_cast<float>(
                reader.GetReal("camera", "freeCamFarPlane", freeCamFarPlane)
                );

            FreeCamMouseSensitivity = static_cast<float>(
                reader.GetReal("camera", "FreeCamMouseSensitivity", FreeCamMouseSensitivity)
                );


        }

        glm::vec3 parseVec3(const std::string& value, const glm::vec3& defaultValue) {
            std::stringstream ss(value);
            float x, y, z;
            char comma; // to consume the commas
            if (ss >> x >> comma >> y >> comma >> z)
                return glm::vec3(x, y, z);
            return defaultValue; // fallback if parsing fails
        }

    };



