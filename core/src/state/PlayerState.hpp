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


    static bool savePlayerState(Player& player, const std::string& playerStateFilePath) {

        json playerStateJson;


        playerStateJson["player"]["position"]["x"] = player.position.GetX();
        playerStateJson["player"]["position"]["y"] = player.position.GetY();
        playerStateJson["player"]["position"]["z"] = player.position.GetZ();

        playerStateJson["player"]["rotation"]["x"] = player.rotation.GetX();
        playerStateJson["player"]["rotation"]["y"] = player.rotation.GetY();
        playerStateJson["player"]["rotation"]["z"] = player.rotation.GetZ();
        playerStateJson["player"]["rotation"]["w"] = player.rotation.GetW();

        playerStateJson["player"]["cameraYaw"] = player.camera.yaw;
        playerStateJson["player"]["cameraPitch"] = player.camera.pitch;

        playerStateJson["player"]["moveSpeed"] = player.moveSpeed;
        playerStateJson["player"]["jumpStrength"] = player.jumpStrength;


        if (std::filesystem::exists(playerStateFilePath)) {
            std::cout << "Warning: file already exists, overwriting: "
                << playerStateFilePath << "\n";
        }
        else {
            std::cout << "Creating new player state file: "
                << playerStateFilePath << "\n";
        }

        std::ofstream file(playerStateFilePath, std::ios::out | std::ios::trunc);
        if (!file) {
            std::cerr << "Error creating player state file!\n";
            return false;
        }


        file << playerStateJson.dump(4);  // Pretty print with 4-space indentation
        std::cout << "user settings saved successfully!\n";

        file.close();

        return 0;

    }

    //TODO add check for when json structure is different then what is expected
    static void loadPlayerState(Player& player, const std::string& playerStateFilePath) {


        std::ifstream file(playerStateFilePath);
        if (!file) {
            std::cout << playerStateFilePath << " does not exist!\n";
            return;
        }

        json playerStateJson;
        file >> playerStateJson;

        player.position.SetX(playerStateJson["player"]["position"]["x"]);
        player.position.SetY(playerStateJson["player"]["position"]["y"]);
        player.position.SetZ(playerStateJson["player"]["position"]["z"]);

        player.rotation.SetX(playerStateJson["player"]["rotation"]["x"]);
        player.rotation.SetY(playerStateJson["player"]["rotation"]["y"]);
        player.rotation.SetZ(playerStateJson["player"]["rotation"]["z"]);
        player.rotation.SetW(playerStateJson["player"]["rotation"]["w"]);

        player.camera.yaw = playerStateJson["player"]["cameraYaw"];
        player.camera.pitch = playerStateJson["player"]["cameraPitch"];

        player.moveSpeed = playerStateJson["player"]["moveSpeed"];
        player.jumpStrength = playerStateJson["player"]["jumpStrength"];

        file.close();

    }


};


