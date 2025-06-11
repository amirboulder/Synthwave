#pragma once
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

std::string settingsFilePath = "data/userSettings.json";

struct UserSettings {
    int windowWidth = 1920;
    int windowHeight = 1080;
};

json userSettingsJson = {
       {"window", {
           {"size", {{"x", 1280}, {"y", 720}}},

       }}
};

UserSettings loadUserSettings() {

    UserSettings settings;

    std::ifstream file(settingsFilePath);
    if (!file) {
        std::cout << settingsFilePath <<" does not exist! loading with default settings \n";

        return settings;
    }

    json userSettingsJson;
    file >> userSettingsJson;

    settings.windowWidth = userSettingsJson["window"]["size"]["x"];
    settings.windowHeight = userSettingsJson["window"]["size"]["y"];
    
    file.close();

    return settings;
}


int saveUserSettings(UserSettings settings) {

    json userSettingsJson;

    std::ifstream existingFile(settingsFilePath);
    if (!existingFile) {
        std::cout << " user setting file does not exist \n";

    }
    else {

        existingFile >> userSettingsJson;
    }
    

    // Write to a JSON file
    std::ofstream file(settingsFilePath);
    if (!file) {
        std::cerr << "Error creating user settings file!\n";
        return 1;
    }

    userSettingsJson["window"]["size"]["x"] = settings.windowWidth;
    userSettingsJson["window"]["size"]["y"] = settings.windowHeight;

    file << userSettingsJson.dump(4);  // Pretty print with 4-space indentation
    std::cout << "user settings saved successfully!\n";

    file.close();

    return 0;

}

