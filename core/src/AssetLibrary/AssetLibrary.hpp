#pragma once 

class AssetLibrary;

struct AssetLibRef {
    AssetLibrary * assetLib;
};

struct ModelEntry {
    std::unique_ptr<ModelSource> source;
    int refCount = 0;
};

typedef std::pair<std::string, ModelEntry> ModelPair;

//TODO FIX HARDCODED PATHS
class AssetLibrary {

public:

    std::string assetsFolder;

    std::unordered_map<std::string, fs::path> modelPaths;
    std::unordered_map<std::string, uint32_t> modelNameToIndex;
    std::vector<ModelEntry> models;

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
    /// Loads the models if not loaded an returns the model id.
    /// </summary>
    uint32_t requestIndex(const std::string& modelName) {
        // Already loaded
        auto it = modelNameToIndex.find(modelName);
        if (it != modelNameToIndex.end())
            return it->second;

        
        auto pathIt = modelPaths.find(modelName);
        if (pathIt == modelPaths.end()) {
            LogError(LOG_APP, "Model %s not found", modelName.c_str());
            return UINT32_MAX; // use max as invalid, not 0
        }

        // Load it
        uint32_t index = models.size();
        ModelEntry entry;
        entry.source = std::make_unique<ModelSource>(ecs, pathIt->second.string().c_str());
        entry.refCount = 1;
        models.push_back(std::move(entry));
        modelNameToIndex[modelName] = index;
        return index;
    }

    std::string generateGridModel(uint32_t size) {

        std::string modelName = "Grid";
        modelName.append(std::to_string(size));

        uint32_t index = models.size();
        ModelEntry entry;
        entry.source = std::make_unique<ModelSource>(ecs, size);
        entry.refCount = 1;
        models.push_back(std::move(entry));
        modelNameToIndex[modelName] = index;

        return modelName;
    }

    //TODO update map to use fs::path
    void scanForFiles(const std::string& folderPath, const std::string& extension, std::map<std::string, std::string> & map) {

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

                map[nameOnly] = fullPath;
            }
        }

        if (ec) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, ERROR "during directory iteration: %s" RESET, ec.message().c_str());
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

