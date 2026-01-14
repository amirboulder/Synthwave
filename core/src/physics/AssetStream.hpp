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

    /// Get the stream
    std::istream& Get() { return mStream; }

    /// Check if stream is valid
    bool IsOpen() const { return mStream.is_open(); }

private:
    std::ifstream mStream;
};
