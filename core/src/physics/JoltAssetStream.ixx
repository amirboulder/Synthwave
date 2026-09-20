module;

#include <fstream>

// IntelliSense only; cl.exe never sees this. Module units skip the PCH, so they
// get JPH types solely from the import, which IntelliSense reports as incomplete.
#ifdef __INTELLISENSE__
#include <Jolt/Jolt.h>
#endif


export module JoltAssetStream;

import Jolt;
import Logger;

/// An istream interface that reads data from a file in the Assets folder
export class AssetStream
{
public:
    /// Constructor
    AssetStream(const char* inFileName, std::ios_base::openmode inOpenMode)
      :  mStream(inFileName, inOpenMode)
    {
        if (!mStream.is_open())
            LogError(LOG_PHYSICS, "AssetStream failed to open file %s", inFileName);
            
    }

    AssetStream(const JPH::String& inFileName, std::ios_base::openmode inOpenMode)
        : AssetStream(inFileName.c_str(), inOpenMode)
    {
        if (!mStream.is_open())
            LogError(LOG_PHYSICS, "AssetStream failed to open file %s", inFileName);
    }

    /// Get the stream
    std::istream& Get() { return mStream; }

    /// Check if stream is valid
    bool IsOpen() const { return mStream.is_open(); }

private:
    std::ifstream mStream;
};
