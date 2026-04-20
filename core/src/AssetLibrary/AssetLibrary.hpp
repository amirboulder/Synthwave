#pragma once 

class AssetLibrary;

struct AssetLibRef {
    AssetLibrary * assetLib;
};



struct MeshAsset {

    Transform transform; //local transform relative to entity's position
    
    uint32_t baseVertex = UINT32_MAX;
    uint32_t firstIndex = UINT32_MAX;
    uint32_t indexCount = 0;
    uint32_t vertexCount = 0;


    uint32_t materialID = 0;
};


//TODO FIX HARDCODED PATHS
class AssetLibrary {

public:

    std::string assetsFolder;

    std::unordered_map<std::string, fs::path> modelPaths;
    std::unordered_map<std::string, std::unique_ptr<Model>> loadedModels;

    std::vector<MeshAsset> meshRegistry;
    std::vector<MaterialSMPL>  materialRegistry;

    SDL_GPUTexture* defaultTexture = nullptr; //white texture
    SDL_GPUTexture* missingTexture = nullptr; //Magenta texture

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

        //Register Geometry Pool
        ecs.component<GeometryPool>();
        ecs.set<GeometryPool>({});
        ecs.get_mut<GeometryPool>().init(ecs);

        meshRegistry.reserve(modelPaths.size());

        scanForRagdolls();

        //Creating defaultTexture for meshes that don't have a texture
        const RenderContext& renderContext = ecs.get<RenderContext>();
        defaultTexture = RenderUtil::createColorTexture(renderContext.device, 0xFFFFFFFF);
        missingTexture = RenderUtil::createColorTexture(renderContext.device, 0xFFFF00FF);

        LogSuccess(LOG_APP, "AssetLibrary Initialized");
    }

    /// <summary>
    /// Calls the Grid constructor in Model.hpp
    /// called by entity factory before createMeshAssets when creating grid meshes.
    /// </summary>
    std::string generateGridModel(uint32_t size) {

        std::string modelName = "Grid";
        modelName.append(std::to_string(size));

        loadedModels[modelName] = std::make_unique<Model>(ecs, size);

        auto itr = loadedModels.find(modelName);

        Model* model = itr->second.get();

        createMeshAssets(model);

        return modelName;
    }


    /// <summary>
    /// Once a model is loaded we call this function to create a mesh asset for every mesh in the model.
    /// Also setting the meshRegistryIndex in each mesh which is used entities requestMeshComponent.
    /// Note we could also just store a vector of MeshAssetIndices in the model and copy it when needed
    /// </summary>
    void createMeshAssets(Model* modelSource) {

        //For each mesh create a entry in mesh registry
        for (Mesh& mesh : modelSource->meshes) {

            MeshAsset meshAsset;

            meshAsset.transform = mesh.transform;

            meshAsset.baseVertex = mesh.baseVertex;
            meshAsset.firstIndex = mesh.firstIndex;
            meshAsset.indexCount = mesh.indexCount;
            meshAsset.vertexCount = mesh.vertexCount;


            meshAsset.materialID = mesh.materialID;

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
    void fillMeshComponent(Model* modelSource, MeshComponent& meshComponent) {

        meshComponent.MeshAssetIndices.reserve(modelSource->meshes.size());

        for (Mesh& mesh : modelSource->meshes) {

            meshComponent.MeshAssetIndices.push_back(mesh.meshRegistryIndex);
        }
    }


    /// <summary>
    /// Returns a pointer to a Model source given its name.
    /// It first checks if the model is already loaded if not it will attempt to load it.
    /// If it cannot fine the source file it will return null
    /// </summary>
    const Model* getModel(const std::string& modelName) {

        auto itr = loadedModels.find(modelName);
        if (itr != loadedModels.end()) {
            return itr->second.get();
        }
        if (VerifyModelPathExistence(modelName)) {
            auto pathItr = modelPaths.find(modelName);
            return loadModel(modelName, pathItr->second.string());
        }
        return nullptr;
    }


    Model* loadModel(const std::string& modelName, const std::string & filepath) {

        auto pathIt = modelPaths.find(modelName);

        loadedModels[modelName] = std::make_unique<Model>(ecs, materialRegistry, defaultTexture, filepath.c_str());

        auto itr = loadedModels.find(modelName);

        Model* model = itr->second.get();

        //creates an entry for in mesh registry for each mesh in this model
        createMeshAssets(model);

        return model;
    }

    /// <summary>
    /// creates MeshComponent which will be attached to an entity.
    /// Will try to load the model if not loaded.
    /// will return a struct with an empty vector if model does not exist.
    /// Used in Entity factory
    /// </summary>
    MeshComponent requestMeshComponent(const std::string& modelName) {

        MeshComponent meshComponent;

        // check if the model is Already loaded
        auto itr = loadedModels.find(modelName);
        if (itr != loadedModels.end()) {

            Model* modelSource = itr->second.get();

            fillMeshComponent(modelSource, meshComponent);

            return meshComponent;
        }


        //check if it in the assets folder at all
        auto pathItr = modelPaths.find(modelName);
        if (pathItr == modelPaths.end()) {
            LogError(LOG_APP, "Model %s source file not found", modelName.c_str());
            return meshComponent;
        }

        Model* modelSource = loadModel(modelName, pathItr->second.string());

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

