#pragma once

class Pipeline {

public:

	SDL_GPUGraphicsPipeline* pipeline = NULL;

	SDL_GPUShader* vertexShader = NULL;
	SDL_GPUShader* fragmentShader = NULL;

	vector <ModelInstance> & models;
	vector<Transform> & transforms;

	Pipeline(vector <ModelInstance>& models, vector<Transform>& transforms)
		: models(models),
		transforms(transforms)
	{

	}

	bool loadVertexShader(
		SDL_GPUDevice* device,
		const std::string& filename,
		Uint32 sampler_count,
		Uint32 uniform_buffer_count,
		Uint32 storage_buffer_count,
		Uint32 storage_texture_count)
	{
		// Load the SPIR-V bytecode from file
		std::vector<Uint8> spirvCode = loadBinaryFile(filename);
		if (spirvCode.empty()) {
			std::cerr << "Failed to load vertex shader: " << filename << std::endl;
			return false;
		}

		SDL_GPUShaderCreateInfo createinfo = {};
		createinfo.num_samplers = sampler_count;
		createinfo.num_storage_buffers = storage_buffer_count;
		createinfo.num_storage_textures = storage_texture_count;
		createinfo.num_uniform_buffers = uniform_buffer_count;
		createinfo.props = 0;
		createinfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
		createinfo.code = spirvCode.data();
		createinfo.code_size = spirvCode.size();
		createinfo.entrypoint = "main";
		createinfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;

		vertexShader = SDL_CreateGPUShader(device, &createinfo);

		if (vertexShader == NULL) {
			SDL_Log("Failed to create vertex shader!");
			return false;
		}
	}

	bool loadFragmentShader(
		SDL_GPUDevice* device,
		const std::string& filename,
		Uint32 sampler_count,
		Uint32 uniform_buffer_count,
		Uint32 storage_buffer_count,
		Uint32 storage_texture_count)
	{

		std::vector<Uint8> spirvCode = loadBinaryFile(filename);
		if (spirvCode.empty()) {
			std::cerr << "Failed to load fragment shader: " << filename << std::endl;
			return false;
		}

		SDL_GPUShaderCreateInfo createinfo = {};
		createinfo.num_samplers = sampler_count;
		createinfo.num_storage_buffers = storage_buffer_count;
		createinfo.num_storage_textures = storage_texture_count;
		createinfo.num_uniform_buffers = uniform_buffer_count;
		createinfo.props = 0;
		createinfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
		createinfo.code = spirvCode.data();
		createinfo.code_size = spirvCode.size();
		createinfo.entrypoint = "main";
		createinfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;

		fragmentShader =  SDL_CreateGPUShader(device, &createinfo);

		if (fragmentShader == NULL) {
			SDL_Log("Failed to create fragment shader!");
			return false;
		}
	}

	// Helper function to load binary file data
	std::vector<Uint8> loadBinaryFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::binary | std::ios::ate);

		if (!file.is_open()) {
			std::cerr << "Failed to open file: " << filename << std::endl;
			return {};
		}

		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		std::vector<Uint8> buffer(size);
		if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
			std::cerr << "Failed to read file: " << filename << std::endl;
			return {};
		}

		return buffer;
	}

};

