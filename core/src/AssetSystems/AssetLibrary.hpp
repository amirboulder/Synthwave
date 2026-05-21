#pragma once 

#include "AssetImporter.hpp"

class AssetLibrary;

struct AssetLibRef {
    AssetLibrary * assetLib;
};



//TODO FIX HARDCODED PATHS

/// <summary>
/// This class is responsible for 
/// </summary>
class AssetLibrary {

public:

    flecs::world& ecs;

    Manifest manifest;

    std::vector<uint64_t> assetIds;

    const std::string assetsFolder;
    

    //std::unordered_map<std::string, fs::path> modelPaths;
    std::vector<fs::path> modelPaths;

    TextureArray diffuseTextures;

    //maps ragdoll names to their paths
    std::map<std::string, std::string> ragdolls;



    AssetLibrary(flecs::world& ecs, const fs::path& manifestPath = "games/CrashTheSim/src/manifest.json", const std::string& assetFolder = "assets")
        :ecs(ecs), assetsFolder(assetFolder)
    {
        const RenderContext& renderContext = ecs.get<RenderContext>();

        scanForFilesRecursive(assetsFolder, ".glb", modelPaths);
       // scanForFilesRecursive(assetsFolder, ".obj", modelPaths);

        // Register the ref component
        ecs.component<AssetLibRef>();
        ecs.set<AssetLibRef>({ this });

        manifest.Load(manifestPath);

        scanForRagdolls();
    
        diffuseTextures.init(renderContext.device);

        //Creating defaultTexture for meshes that don't have a texture
        SDL_Surface* imageData = RenderUtil::LoadImage("assets/checkerboard.bmp", 4);
        if (imageData == NULL)
        {
            LogError(LOG_RENDER, "Could not load checkerboard.bmp image data!");
        }

       RenderUtil::uploadToTextureArray(renderContext.device, diffuseTextures, imageData);



        LogSuccess(LOG_APP, "AssetLibrary Initialized");
    }


    /// <summary>
    /// For every gltf filePath in assets folder check if the last write is newer that manifestLastWrite,
    /// this (usually) means that the file is either new or has been updated.
    /// If the check is true then hash the file and compare it against sourceHashes in the manifest.
    /// If no matches found then the file is either new or has been updated.
    /// If file source path is present then then the file has been updated.
    /// IF not then its a new file.
    /// </summary>
    void importAssets(Manifest & manifest) {

        fs::file_time_type manifestLastWrite = fs::last_write_time(manifest.Path());

        std::vector<fs::path> needsImport;
        std::vector<fs::path> needsReImport;

        for (const fs::path& filePath : modelPaths) {

            fs::file_time_type fileLastWrite = fs::last_write_time(filePath);

            if (fileLastWrite > manifestLastWrite) {
                     
                uint64_t contentHash = util::generateContentHash(filePath);

                bool contentHashFound = false;
                bool pathFound = false;
                manifest.ForEach([&](const AssetMetadata& meta) {

                    if (meta.sourceHash == contentHash) contentHashFound = true;
                    if (meta.sourcePath == filePath.string()) pathFound = true;


                });

                if (!contentHashFound && !pathFound) needsImport.push_back(filePath);     // new file
                if (!contentHashFound && pathFound)  needsReImport.push_back(filePath);   // modified

                if (contentHashFound == true && pathFound == false) {
                    
                    LogWarn(LOG_APP, "file %s has been renamed ", filePath.string().c_str());
                }
            }

        }

        for (const fs::path& filePath : needsImport) {
            AssetImporter::ImportGLTF(filePath, assetsFolder, manifest);
        }

        for (const fs::path& filePath : needsReImport) {
            AssetImporter::reImportGLTF(filePath, assetsFolder, manifest);
        }


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

    void scanForFilesRecursive(const std::string& folderPath, const std::string& extension, std::vector<fs::path>& filePaths) {

        std::error_code ec;

        if (!fs::exists(folderPath, ec) || ec) {
            LogError(LOG_APP, "Directory %s doesn't exist or can't be accessed", folderPath.c_str());
            //TODO FAIL TO LOAD LEVEL 
        }

        for (const auto& entry : fs::recursive_directory_iterator(folderPath)) {
            if (!entry.is_regular_file()) continue;
            if (entry.path().extension() != extension) continue;

            filePaths.push_back(entry);
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

