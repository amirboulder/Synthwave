#pragma once 

class AssetLibrary;

struct AssetLibRef {
    AssetLibrary * assetLib;
};

struct MeshAsset {

    Transform transform; //local transform relative to entity's position
    
    SDL_GPUBuffer* transformsBuffer = nullptr;

    SDL_GPUBuffer* vertexBuffer = nullptr;
    SDL_GPUBuffer* indexBuffer = nullptr;
    
    SDL_GPUTexture* diffuseTexture = nullptr;

    uint32_t numIndices = 0;

};


//TODO FIX HARDCODED PATHS
class AssetLibrary {

public:

    std::string assetsFolder;

    std::unordered_map<std::string, fs::path> modelPaths;
    std::unordered_map<std::string, std::unique_ptr<ModelSource>> loadedModels;
    std::vector<MeshAsset> meshRegistry;

    //maps ragdoll names to their paths
    std::map<std::string, std::string> ragdolls;

    flecs::world& ecs;

    AssetLibrary(flecs::world& ecs, std::string assetFolder = "assets")
        :ecs(ecs), assetsFolder(assetFolder)
    {
        scanForFilesRecursive(assetsFolder, ".glb", modelPaths);
        scanForFilesRecursive(assetsFolder, ".obj", modelPaths);

        // Register the ref component
        ecs.component<AssetLibRef>();
        ecs.set<AssetLibRef>({ this });

        scanForRagdolls();

        LogSuccess(LOG_APP, "AssetLibrary Initialized");
    }

    /// <summary>
    /// Calls the Grid constructor in Model.hpp
    /// called by entity factory before createMeshAssets when creating grid meshes.
    /// </summary>
    std::string generateGridModel(uint32_t size) {

        std::string modelName = "Grid";
        modelName.append(std::to_string(size));

        loadedModels[modelName] = std::make_unique<ModelSource>(ecs, size);

        auto itr = loadedModels.find(modelName);

        ModelSource* modelSource = itr->second.get();

        createMeshAssets(modelSource);

        return modelName;
    }


    /// <summary>
    /// Once a model is loaded we call this function to create a mesh asset for every mesh in the model.
    /// Also setting the meshRegistryIndex in each mesh which is used entities requestMeshComponent.
    /// Note we could also just store a vector of MeshAssetIndices in the model and copy it when needed
    /// </summary>
    void createMeshAssets(ModelSource* modelSource) {

        //For each mesh create a entry in mesh registry
        for (MeshSource& mesh : modelSource->meshes) {

            MeshAsset meshAsset;

            meshAsset.transform = mesh.transform;

            meshAsset.vertexBuffer = mesh.vertexBuffer;
            meshAsset.indexBuffer = mesh.indexBuffer;
            meshAsset.numIndices = mesh.size;

            meshAsset.diffuseTexture = mesh.diffuseTexture;

            meshRegistry.push_back(meshAsset);

            uint32_t meshRegistryIndex = meshRegistry.size() - 1;

            mesh.meshRegistryIndex = meshRegistryIndex;
        }

    }


    /// <summary>
    /// Used when an entity requests a mesh components.
    /// We fill the vector with meshRegistryIndex from each mesh.
    /// Note we could also just store a vector of MeshAssetIndices in the model and copy it when needed.
    /// </summary>
    void fillMeshComponent(ModelSource* modelSource, MeshComponent& meshComponent) {

        meshComponent.MeshAssetIndices.reserve(modelSource->meshes.size());

        for (MeshSource& mesh : modelSource->meshes) {

            meshComponent.MeshAssetIndices.push_back(mesh.meshRegistryIndex);
        }
    }


    /// <summary>
    /// Returns a pointer to a Model source given its name.
    /// It first checks if the model is already loaded if not it will attempt to load it.
    /// If it cannot fine the source file it will return null
    /// </summary>
    ModelSource* getModel(const std::string& modelName) {

        ModelSource* modelSourcePtr = nullptr;

        auto itr = loadedModels.find(modelName);
        if (itr != loadedModels.end()) {

            modelSourcePtr = itr->second.get();
            
        }

        if (VerifyModelPathExistence(modelName)) {

            auto pathItr = modelPaths.find(modelName);
            modelSourcePtr = loadModel(modelName, pathItr->second.string());
        }

        return modelSourcePtr;
    }


    ModelSource* loadModel(const std::string& modelName, const std::string filepath) {

        auto pathIt = modelPaths.find(modelName);

        loadedModels[modelName] = std::make_unique<ModelSource>(ecs, filepath.c_str());

        auto itr = loadedModels.find(modelName);

        ModelSource* modelSource = itr->second.get();

        //creates an entry for in mesh registry for each mesh in this model
        createMeshAssets(modelSource);

        return modelSource;
    }

    /// <summary>
    /// creates MeshComponent which will be attached to an entity.
    /// Will try to load the model if not loaded.
    /// will return a struct with an empty vector if model does not exist.
    /// Used in Entity factory
    /// </summary>
    /// <param name="modelName"></param>
    /// <returns></returns>
    MeshComponent requestMeshComponent(const std::string& modelName) {

        MeshComponent meshComponent;

        // check if the model is Already loaded
        auto itr = loadedModels.find(modelName);
        if (itr != loadedModels.end()) {

            ModelSource* modelSource = itr->second.get();

            fillMeshComponent(modelSource, meshComponent);

            return meshComponent;
        }


        //check if it in the assets folder at all
        auto pathItr = modelPaths.find(modelName);
        if (pathItr == modelPaths.end()) {
            LogError(LOG_APP, "Model %s source file not found", modelName.c_str());
            return meshComponent;
        }

        ModelSource* modelSource = loadModel(modelName, pathItr->second.string());

        fillMeshComponent(modelSource, meshComponent);
        return meshComponent;
    }

    
    bool VerifyModelPathExistence(const std::string& modelName) {

        auto pathItr = modelPaths.find(modelName);
        if (pathItr == modelPaths.end()) {
            LogError(LOG_APP, "Model %s source file not found", modelName.c_str());
            return false;
        }

        return true;
    }

    //TODO update ragdolls map to use fs::path and deprecate this
    void scanForFiles(const std::string& folderPath, const std::string& extension, std::map<std::string, std::string> & map) {

        std::error_code ec;

        if (!fs::exists(folderPath, ec) || ec) {
            LogError(LOG_APP, "Directory %s doesn't exist or can't be accessed", folderPath.c_str());
        }

        for (const auto& entry : fs::directory_iterator(folderPath, ec)) {
            if (ec) {
                LogError(LOG_APP, "iterating directory: %s", ec.message().c_str());
                break;
            }

            if (entry.is_regular_file(ec) && !ec && entry.path().extension() == extension) {
                std::string nameOnly = entry.path().stem().string();

                // using generic_string() for consistent forward slashes
                std::string fullPath = entry.path().generic_string();

                map[nameOnly] = fullPath;
            }
        }

        if (ec) {
            LogError(LOG_APP, "during directory iteration: %s", ec.message().c_str());
        }
    }

    void scanForFilesRecursive(const std::string& folderPath, const std::string& extension, std::unordered_map<std::string, fs::path>& map) {

        std::error_code ec;

        if (!fs::exists(folderPath, ec) || ec) {
            LogError(LOG_APP, "Directory %s doesn't exist or can't be accessed", folderPath.c_str());
            //TODO FAIL TO LOAD LEVEL 
        }

        for (const auto& entry : fs::recursive_directory_iterator(folderPath)) {
            if (!entry.is_regular_file()) continue;
            if (entry.path().extension() != extension) continue;

            std::string nameOnly = entry.path().stem().string();

            map[nameOnly] = entry;
        }

        if (ec) {
            LogError(LOG_APP, "during directory iteration: %s", ec.message().c_str());
        }
    }

    void scanForRagdolls() {

        scanForFiles(util::getBuildRagdollsFolder().string(), ".bof", ragdolls);
    }

    //TODO
    void release(const std::string& id) {
   
    }

    // TODO Call on level unload / scene transition
    void cleanup() {
       
    }
};

