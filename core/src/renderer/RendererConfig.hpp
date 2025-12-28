#pragma once
#include "core/src/pch.h"

//TODO take out freeCam pos/front and put it in the save file
class RenderConfig {
public:

	SDL_GPUSampleCount sampleCount = SDL_GPU_SAMPLECOUNT_8;
	SDL_GPUSwapchainComposition colorspace = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;
	SDL_GPUPresentMode presentMode = SDL_GPU_PRESENTMODE_VSYNC;

	uint32_t windowWidth = 1920;
	uint32_t windowHeight = 1080;

	bool RenderPhysics = true;
	bool DrawBoundingBoxPhysics = false;
	bool DrawShapeWireframePhysics = true;


	// TODO seprate into camera config struct for camera params
	glm::vec3 FreeCamPos = glm::vec3(-2.0f, -2.0f, 0.0f);
	glm::vec3 FreeCamFront = glm::vec3(-2.0f, -2.0f, 0.0f);
	float FreeCamYaw = 0.0f;
	float FreeCamPitch = 0.0f;
	float freeCamFov = 45.0f;
	float freeCamNearPlane = 0.1f;
	float freeCamFarPlane = 1000.0f;
	float FreeCamMouseSensitivity = 0.1;

	RenderConfig() {

	}


	static void saveRendererConfigINIFile(flecs::world& ecs,const std::string& filepath) {

		std::ofstream iniFile(filepath);
		if (!iniFile) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create INI file: %s", filepath.c_str());
		}

		const RenderConfig& cfg = ecs.get<RenderConfig>();

		iniFile << "[window]\n";
		iniFile << "width=" << cfg.windowWidth << "\n";
		iniFile << "height=" << cfg.windowHeight << "\n\n";


		iniFile << "[renderer]\n";
		iniFile << "sampleCountMSAA=" << static_cast<int>(cfg.sampleCount) << "\n";
		iniFile << "RendererPhysics=" << (cfg.RenderPhysics ? "true" : "false") << "\n";
		iniFile << "DrawBoundingBoxPhysics=" << (cfg.DrawBoundingBoxPhysics ? "true" : "false") << "\n";
		iniFile << "DrawShapeWireframePhysics=" << (cfg.DrawShapeWireframePhysics ? "true" : "false") << "\n\n";

		iniFile << "[camera]\n";
		iniFile << "FreeCamPos=" << cfg.FreeCamPos.x << "," << cfg.FreeCamPos.y << "," << cfg.FreeCamPos.z << "\n";
		iniFile << "FreeCamFront=" << cfg.FreeCamFront.x << "," << cfg.FreeCamFront.y << "," << cfg.FreeCamFront.z << "\n";
		iniFile << "FreeCamYaw=" << cfg.FreeCamYaw << "\n";
		iniFile << "FreeCamPitch=" << cfg.FreeCamPitch << "\n";
		iniFile << "freeCamFov=" << cfg.freeCamFov << "\n";
		iniFile << "freeCamNearPlane=" << cfg.freeCamNearPlane << "\n";
		iniFile << "freeCamFarPlane=" << cfg.freeCamFarPlane << "\n";

		iniFile << "FreeCamMouseSensitivity=" << cfg.FreeCamMouseSensitivity << "\n";


		iniFile.close();

		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Renderer config saved to: %s", filepath.c_str());

	}

	//load render config values from ini file
	//sets values to default if they are not found within the config file
	static void loadRendererConfigINIFile(flecs::world& ecs,const std::string& filepath) {

		//if (!std::filesystem::exists(filepath)) {
		//	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "renderConfig file does not exist. Creating a new one");

		//	RendererConfig::saveRendererConfigINIFile(ecs,filepath);

		//}

		INIReader reader(filepath);

		if (reader.ParseError() < 0) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load file: %s loading using default settings", filepath.c_str());
			return;
		}

		const RenderConfig defaultCfg;

		RenderConfig& cfg = ecs.get_mut<RenderConfig>();

		cfg.windowWidth = reader.GetInteger("window", "width", defaultCfg.windowWidth);
		cfg.windowHeight = reader.GetInteger("window", "height", defaultCfg.windowHeight);

		cfg.sampleCount = static_cast<SDL_GPUSampleCount>(
			reader.GetInteger("renderer", "sampleCountMSAA", defaultCfg.sampleCount)
			);

		cfg.RenderPhysics = reader.GetBoolean("renderer", "RendererPhysics", defaultCfg.RenderPhysics);
		cfg.DrawBoundingBoxPhysics = reader.GetBoolean("renderer", "DrawBoundingBoxPhysics", defaultCfg.DrawBoundingBoxPhysics);
		cfg.DrawShapeWireframePhysics = reader.GetBoolean("renderer", "DrawShapeWireframePhysics", defaultCfg.DrawShapeWireframePhysics);


		cfg.FreeCamPos = RenderConfig::parseVec3(
			reader.Get("camera", "FreeCamPos", "-2,-2,0"),
			defaultCfg.FreeCamPos
		);

		cfg.FreeCamFront = RenderConfig::parseVec3(
			reader.Get("camera", "FreeCamFront", "-2,-2,0"),
			defaultCfg.FreeCamFront
		);

		cfg.FreeCamYaw = static_cast<float>(
			reader.GetReal("camera", "FreeCamYaw", defaultCfg.FreeCamYaw)
			);

		cfg.FreeCamPitch = static_cast<float>(
			reader.GetReal("camera", "FreeCamPitch", defaultCfg.FreeCamPitch)
			);

		cfg.freeCamFov = static_cast<float>(
			reader.GetReal("camera", "freeCamFov", defaultCfg.freeCamFov)
			);

		cfg.freeCamNearPlane = static_cast<float>(
			reader.GetReal("camera", "freeCamNearPlane", defaultCfg.freeCamNearPlane)
			);

		cfg.freeCamFarPlane = static_cast<float>(
			reader.GetReal("camera", "freeCamFarPlane", defaultCfg.freeCamFarPlane)
			);

		cfg.FreeCamMouseSensitivity = static_cast<float>(
			reader.GetReal("camera", "FreeCamMouseSensitivity", defaultCfg.FreeCamMouseSensitivity)
			);


	}

	static glm::vec3 parseVec3(const std::string& value, const glm::vec3& defaultValue) {
		std::stringstream ss(value);
		float x, y, z;
		char comma; // to consume the commas
		if (ss >> x >> comma >> y >> comma >> z)
			return glm::vec3(x, y, z);
		return defaultValue; // fallback if parsing fails
	}

};



