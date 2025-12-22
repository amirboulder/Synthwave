#pragma once

#include "core/src/pch.h"

/// An istream interface that reads data from a file in the Assets folder
class AssetStream
{
public:
    /// Constructor
    AssetStream(const char* inFileName, std::ios_base::openmode inOpenMode)
      :  mStream(inFileName, inOpenMode)
    {
        if (!mStream.is_open())
            cout << "ERROR: Failed to open file " << inFileName << "\n";
    }

    AssetStream(const JPH::String& inFileName, std::ios_base::openmode inOpenMode)
        : AssetStream(inFileName.c_str(), inOpenMode)
    {
        if (!mStream.is_open())
            cout << "ERROR: Failed to open file " << inFileName << "\n";
    }

    /// Get the path to the assets folder
    static JPH::String sGetAssetsBasePath() {
        // Simple implementation: look for Assets folder relative to executable
        std::filesystem::path current = std::filesystem::current_path();

        // Try current directory first
        if (std::filesystem::exists(current / "assets"))
            return JPH::String((current / "assets" / "").string());

        // Try parent directories
        std::filesystem::path search = current;
        for (int i = 0; i < 5; ++i) {
            search = search.parent_path();
            if (std::filesystem::exists(search / "assets"))
                return JPH::String((search / "assets" / "").string());
        }

        // Default to current directory + Assets/
        return "assets/";
    }

    /// Get the stream
    std::istream& Get() { return mStream; }

    /// Check if stream is valid
    bool IsOpen() const { return mStream.is_open(); }

private:
    std::ifstream mStream;
};
