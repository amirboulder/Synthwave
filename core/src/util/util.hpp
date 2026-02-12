#pragma once

void check_error_bool(const bool res)
{
    if (!res) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", SDL_GetError());
    }
}

template<typename T>
T* check_error_ptr(T* ptr) {
    if (!ptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", SDL_GetError());
    }
    return ptr;
}


glm::vec3 JPHVec3ToGLM(JPH::Vec3Arg vec) {

    return glm::vec3(vec.GetX(), vec.GetY(), vec.GetZ());
}

JPH::Vec3 GLMVec3ToJPH(const glm::vec3 vec) {

    return JPH::Vec3(vec.x, vec.y, vec.z);
}


glm::quat JPHQuatToGLM(JPH::QuatArg quat) {

    return glm::quat(quat.GetW(), quat.GetX(), quat.GetY(), quat.GetZ()); // w, x, y, z order      
}

JPH::Quat GLMQuatToJPH(const glm::quat quat) {

    return JPH::Quat(quat.x, quat.y, quat.z, quat.w); // x, y, z, w order
}


namespace util {


	fs::path getRepoRoot() {

		fs::path path = fs::path(__FILE__).lexically_normal();
		fs::path repoRoot = path.parent_path().parent_path().parent_path().parent_path();

		return repoRoot;
	}


	fs::path getRepoAssetsFolder() {

		fs::path repoRoot = getRepoRoot();
		fs::path assetsPath = repoRoot / "assets";

		return assetsPath;
	}


	fs::path getRepoRagdollsFolder() {

		fs::path assetsPath = getRepoAssetsFolder();
		fs::path ragdollsPath = assetsPath / "ragdolls";

		return ragdollsPath;
	}

	fs::path getCurrentPath() {

		fs::path cwd = std::filesystem::current_path();

		return cwd;
	}

	fs::path getBuildAssetsFolder() {

		fs::path cwd = getCurrentPath();
		fs::path assetsPath = cwd / "assets";

		return assetsPath;
	}

	fs::path getBuildRagdollsFolder() {

		fs::path assetsPath = getBuildAssetsFolder();
		fs::path ragdollsPath = assetsPath / "ragdolls";

		return ragdollsPath;
	}


	bool saveDataToFile(std::stringstream& dataOut, const std::filesystem::path& filePath) {

		// Create parent directories if they don't exist
		std::filesystem::create_directories(filePath.parent_path());

		std::ofstream file(filePath, std::ios::binary);
		if (!file.is_open()) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, ERROR "Cannot open output file : %s" RESET, filePath.c_str());
			return false;
		}

		file << dataOut.str();

		if (file.fail()) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, ERROR "Writing to file: %s" RESET, filePath.c_str());
			return false;
		}

		return true;
	}

}

