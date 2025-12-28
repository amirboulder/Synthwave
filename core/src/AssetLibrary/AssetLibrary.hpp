#pragma once 

#include "core/src/pch.h"

class AssetLibrary;

struct AssetLibRef {
    AssetLibrary * assetLib;
};

class AssetLibrary {

public:


    std::string assetFolder;

    std::unordered_map<std::string, std::unique_ptr<ModelSource>> models;

    flecs::world& ecs;

    AssetLibrary(flecs::world& ecs, std::string assetFolder = "assets")
        :ecs(ecs), assetFolder(assetFolder)
    {
        defaultAssets();

        // Register the ref component
        ecs.component<AssetLibRef>();
        ecs.set<AssetLibRef>({ this });

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


    //TODO fix
    ~AssetLibrary() {
        for (auto& [id, model] : models) {
            //delete model;
        }
        models.clear();
    }
};

