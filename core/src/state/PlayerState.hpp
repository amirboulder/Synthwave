#pragma once

#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

#include <Jolt/Jolt.h>

#include "../player.hpp"

using json = nlohmann::json;

//std::string playerStateFilePath = "data/playerState.json";

json playerStateJson =
{
       {"player", {
           {"position", {{"x", 1}, {"y", -15.0}, {"y", 0} }},
           {"rotation", {{"x", 0}, {"y", 0}, {"y", 0}, {"w", 1} }},

           {"cameraYaw", 0},
           {"cameraPitch", 0},

           {"moveSpeed", 10.1},
           {"jumpStrength", -7.5},

       }}
};

class PlayerState {


public:


    static bool savePlayerState(Player& player, flecs::world& ecs,const std::string& playerStateFilePath) {

        Camera& camera = ecs.lookup("PlayerCam").get_mut<Camera>();

        json playerStateJson;

        playerStateJson["player"]["position"]["x"] = player.position.GetX();
        playerStateJson["player"]["position"]["y"] = player.position.GetY();
        playerStateJson["player"]["position"]["z"] = player.position.GetZ();

        playerStateJson["player"]["rotation"]["x"] = player.rotation.GetX();
        playerStateJson["player"]["rotation"]["y"] = player.rotation.GetY();
        playerStateJson["player"]["rotation"]["z"] = player.rotation.GetZ();
        playerStateJson["player"]["rotation"]["w"] = player.rotation.GetW();

        playerStateJson["player"]["cameraYaw"] = camera.yaw;
        playerStateJson["player"]["cameraPitch"] = camera.pitch;

        playerStateJson["player"]["moveSpeed"] = player.moveSpeed;
        playerStateJson["player"]["jumpStrength"] = player.jumpStrength;


        if (std::filesystem::exists(playerStateFilePath)) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Warning: file already exists, overwriting: %s ", playerStateFilePath.c_str());
        }
        else {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Creating new player state file : %s ", playerStateFilePath.c_str());
        }

        std::ofstream file(playerStateFilePath, std::ios::out | std::ios::trunc);
        if (!file) {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Error creating player state file!");
            return false;
        }


        file << playerStateJson.dump(4);  // Pretty print with 4-space indentation
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "saved player state");

        file.close();

        return 0;

    }

    //TODO add check for when json structure is different then what is expected
    static void loadPlayerState(Player& player, flecs::world& ecs,const std::string& playerStateFilePath) {

        Camera& camera = ecs.lookup("PlayerCam").get_mut<Camera>();

        std::ifstream file(playerStateFilePath);
        if (!file) {
            std::cout << playerStateFilePath << " does not exist!\n";
            return;
        }

        json playerStateJson;

        try {

            file >> playerStateJson;

        player.position.SetX(playerStateJson.at("player").at("position").at("x"));
        player.position.SetY(playerStateJson.at("player").at("position").at("y"));
        player.position.SetZ(playerStateJson.at("player").at("position").at("z"));

        player.rotation.SetX(playerStateJson.at("player").at("rotation").at("x"));
        player.rotation.SetY(playerStateJson.at("player").at("rotation").at("y"));
        player.rotation.SetZ(playerStateJson.at("player").at("rotation").at("z"));
        player.rotation.SetW(playerStateJson.at("player").at("rotation").at("w"));

        player.moveSpeed = playerStateJson.at("player").at("moveSpeed");
        player.jumpStrength = playerStateJson.at("player").at("jumpStrength");

        camera.yaw = playerStateJson.at("player").at("cameraYaw");
        camera.pitch = playerStateJson.at("player").at("cameraPitch");

        // updating the camera in players contructor because it was created with default values before 
        // player state was loaded, updating it here avoid a camera jump in the first frame as it is rendered before
        // enough time delta time is accumulated for player.update( and other update functions) to be called.
        glm::vec3 characterPosGLM = glm::vec3(player.position.GetX(), player.position.GetY(), player.position.GetZ());
        camera.position = characterPosGLM + player.offset;
        camera.updateVectors();

        file.close();

        }
        catch (const json::exception& e) {
           
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error loading player state from : %s Error code : %d", playerStateFilePath.c_str(), e.id);
            return;
        }
    }
};


