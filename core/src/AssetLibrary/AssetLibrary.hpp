#pragma once 

#include "core/src/pch.h"

class AssetLibrary;

struct AssetLibRef {
    AssetLibrary * assetLib;
};

//TODO FIX HARDCODED PATHS
class AssetLibrary {

public:

    std::string assetsFolder;

    std::unordered_map<std::string, std::unique_ptr<ModelSource>> models;

    //maps ragdoll names to their paths
    std::map<std::string, std::string> ragdolls;

    flecs::world& ecs;

    AssetLibrary(flecs::world& ecs, std::string assetFolder = "assets")
        :ecs(ecs), assetsFolder(assetFolder)
    {
        defaultAssets();

        // Register the ref component
        ecs.component<AssetLibRef>();
        ecs.set<AssetLibRef>({ this });


        scanForFiles("assets/ragdolls", ".bof");

        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, GOOD "AssetLibrary Initialized" RESET);
    }

    ModelSource* get(const std::string& id) {
        auto it = models.find(id);
        return (it != models.end()) ? it->second.get() : nullptr;
    }

    void add(const std::string& id, std::unique_ptr<ModelSource> model) {
        models[id] = std::move(model);
    }

	void defaultAssets() {

        add("robot", std::make_unique<ModelSource>(ecs, "assets/robot4Wheels.glb"));
        add("CapsuleModel", std::make_unique<ModelSource>(ecs, "assets/capsule4.glb"));
        add("CubeModel", std::make_unique<ModelSource>(ecs, "assets/Cube.glb"));
        add("Grid256", std::make_unique<ModelSource>(ecs, 256, 256));
        add("Mountains", std::make_unique<ModelSource>(ecs, "assets/mtn2.obj", true));
        add("ActorModel", std::make_unique<ModelSource>(ecs, "assets/enemy1.glb"));

        //sponza
        //ModelSource sponzaSource("assets/Sponza/sponza.obj", renderer.context.device);
	}


    void scanForFiles(const std::string& folderPath, const std::string& extension) {

        std::error_code ec;

        if (!fs::exists(folderPath, ec) || ec) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, ERROR "Directory %s doesn't exist or can't be accessed" RESET, folderPath.c_str());
        }

        for (const auto& entry : fs::directory_iterator(folderPath, ec)) {
            if (ec) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, ERROR "iterating directory: %s" RESET, ec.message().c_str());
                break;
            }

            if (entry.is_regular_file(ec) && !ec && entry.path().extension() == extension) {
                std::string nameOnly = entry.path().stem().string();

                // using generic_string() for consistent forward slashes
                std::string fullPath = entry.path().generic_string();

                ragdolls[nameOnly] = fullPath;
            }
        }

        if (ec) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, ERROR "during directory iteration: %s" RESET, ec.message().c_str());
        }
    }


    //TODO fix
    ~AssetLibrary() {
        for (auto& [id, model] : models) {
            //delete model;
        }
        models.clear();
    }
};

