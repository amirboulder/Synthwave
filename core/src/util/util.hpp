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

// Converts a quaternion rotation to a forward direction vector (0, 0, -1) rotated by the quaternion
glm::vec3 quatToDirection(const glm::quat& q)
{
	// Rotate the default forward vector (-Z in OpenGL/GLM convention)
	return glm::normalize(q * glm::vec3(0.0f, 0.0f, -1.0f));
}


inline glm::quat rotationFromEulerDegrees(const glm::vec3& eulerDeg)
{
	return glm::normalize(glm::quat(glm::vec3(
		glm::radians(eulerDeg.x),
		glm::radians(eulerDeg.y),
		glm::radians(eulerDeg.z)
	)));
}



JPH::Quat dirToQuat(const JPH::Vec3& dir) {
	JPH::Vec3 forward = JPH::Vec3(0, 0, 1); // Jolt's default forward
	JPH::Vec3 d = dir.Normalized();

	float dot = forward.Dot(d);

	if (dot > 0.9999f)
		return JPH::Quat::sIdentity();

	if (dot < -0.9999f) {
		// 180° flip around up axis
		return JPH::Quat::sRotation(JPH::Vec3(0, 1, 0), JPH::JPH_PI);
	}

	JPH::Vec3 axis = forward.Cross(d).Normalized();
	float angle = acosf(dot);
	return JPH::Quat::sRotation(axis, angle);
}

template <typename F>
class scope_exit {
	F f_;
public:
	explicit scope_exit(F&& f) : f_(std::forward<F>(f)) {}
	~scope_exit() { f_(); }

	// Delete copy operations to prevent multiple executions
	scope_exit(const scope_exit&) = delete;
	scope_exit& operator=(const scope_exit&) = delete;
};

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


	uint64_t Now() {
		return static_cast<uint64_t>(
			std::chrono::duration_cast<std::chrono::seconds>(
				std::chrono::system_clock::now().time_since_epoch()
			).count()
			);
	}


	bool saveDataToFile(std::stringstream& dataOut, const std::filesystem::path& filePath) {

		// Create parent directories if they don't exist
		std::filesystem::create_directories(filePath.parent_path());

		std::ofstream file(filePath, std::ios::binary);
		if (!file.is_open()) {
			LogError(LOG_ERR, "Cannot open output file : %s", filePath.c_str());
			return false;
		}

		file << dataOut.str();

		if (file.fail()) {
			LogError(LOG_ERR, "Writing to file: %s", filePath.c_str());
			return false;
		}

		return true;
	}

	//Generates a DETERMINISTIC 64bit ID
	//God does not play dice with the universe
	uint64_t generateAssetID(std::string_view assetName) {

		return XXH3_64bits(assetName.data(), assetName.size());
	}

	/// Even with seed 0 this will produce a different hash than generateAssetID.
	uint64_t generateAssetIDWithSeed(std::string_view assetName, uint64_t seed) {

		return XXH3_64bits_withSeed(assetName.data(), assetName.size(), seed);
	}

	uint64_t generateRandomAssetID(std::string_view assetName) {

		//generate a good random number, Thanks CHERNO!
		static std::random_device s_randomDevice;
		static std::mt19937_64 s_engine(s_randomDevice());
		static std::uniform_int_distribution<uint64_t> s_uniformDistribution;
		
		return XXH3_64bits_withSeed(assetName.data(), assetName.size(), s_uniformDistribution(s_engine));
	}

	uint64_t generateContentHash(const fs::path& filePath) {

		if (!fs::exists(filePath)) {
			LogError(LOG_RENDER, "File: %s does not exist, cannot create content hash", filePath.string().c_str());
			return 0;
		}

		std::ifstream file(filePath, std::ios::binary);
		if (!file.is_open()) {
			LogError(LOG_RENDER, "Cannot open file: %s for creating content hash", filePath.string().c_str());
			return 0;
		}

		file.seekg(0, std::ios::end);
		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		std::vector<char> buffer(size);
		if (!file.read(buffer.data(), size)) {
			LogError(LOG_RENDER, "Failed to read file: %s for content hash", filePath.string().c_str());
			return 0;
		}

		return XXH3_64bits(buffer.data(), buffer.size());
	}
}

