#pragma once

struct Context {
	SDL_GPUDevice* device;
	SDL_Window* window;
	SDL_GPUCommandBuffer* commandBuffer;
	SDL_GPUTexture* swapchainTexture;

	SDL_GPUSampleCount sampleCountMSAA;
};

struct RenderContext {
	SDL_GPUDevice* device = NULL;
	SDL_Window* window = NULL;
};

struct FrameContext {
	SDL_GPUCommandBuffer* commandBuffer;
	SDL_GPUTexture* swapchainTexture;
};



struct FrameDataUniforms {
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 viewProjection;
	glm::vec3 cameraPos;
	float _pad;
};

struct PerModelUniforms {
    glm::mat4 model;
    glm::mat4 mvp;
};

struct SceneNodeData {
	std::string name;
	uint64_t meshID;
	Transform transform;
};

struct SceneHeader {

	uint32_t magic = 0x5343454e45;    //'SCENE' for format validation on load
	uint32_t nodesNum;
	uint64_t assetID;
};

class RenderUtil {


public:

	static void uploadBufferData(SDL_GPUDevice* device, SDL_GPUBuffer* & buffer, const void* data, size_t size, Uint32 SDL_GPUBufferUsageFlag) {

        //create buffer
        SDL_GPUBufferCreateInfo bufferCreateInfo = {};
        bufferCreateInfo.usage = SDL_GPUBufferUsageFlag;
        bufferCreateInfo.size = size;

        buffer = SDL_CreateGPUBuffer(device, &bufferCreateInfo);
        if (!buffer) {
            LogError(LOG_RENDER, "Failed to create buffer of type: %d  %s", SDL_GPUBufferUsageFlag, SDL_GetError());
            return ;
        }

        // Create transfer buffer
        SDL_GPUTransferBufferCreateInfo transferInfo = {};
        transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transferInfo.size = size;

        SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
        if (!transferBuffer) {

			LogError(LOG_RENDER, "Failed to create transfer buffer: %s", SDL_GetError());
            return;
        }

        // Map and copy data
        void* mappedData = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
        if (!mappedData) {

			LogError(LOG_RENDER, "Failed to map transfer buffer: %s", SDL_GetError());
            SDL_ReleaseGPUTransferBuffer(device, transferBuffer);

            return;
        }

        memcpy(mappedData, data, size);
        SDL_UnmapGPUTransferBuffer(device, transferBuffer);

        // Upload to GPU
        SDL_GPUCommandBuffer* cmdBuf = SDL_AcquireGPUCommandBuffer(device);
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmdBuf);

        SDL_GPUTransferBufferLocation srcLoc = { transferBuffer, 0 };
        SDL_GPUBufferRegion dstRegion = { buffer, 0, size };
        SDL_UploadToGPUBuffer(copyPass, &srcLoc, &dstRegion, false);

        SDL_EndGPUCopyPass(copyPass);
        SDL_SubmitGPUCommandBuffer(cmdBuf);
        SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
    }


    // Helper function to load binary file data
    static std::vector<Uint8> loadBinaryFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);

        if (!file.is_open()) {

			LogError(LOG_RENDER, "Failed to open file: %s", filename.c_str());
            return {};
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<Uint8> buffer(size);
        if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {

			LogError(LOG_RENDER, "Failed to read file: %s", filename.c_str());
            return {};
        }

        return buffer;
    }

	static SDL_Surface* LoadImage(const char* path, int desiredChannels)
	{
		SDL_Surface* result;
		SDL_PixelFormat format;

		result = SDL_LoadBMP(path);
		if (result == NULL)
		{
			LogError(LOG_RENDER, "Failed to load BMP: %s", SDL_GetError());
			return NULL;
		}

		if (desiredChannels == 4)
		{
			format = SDL_PIXELFORMAT_ABGR8888;
		}
		else
		{
			SDL_assert(!"Unexpected desiredChannels");
			SDL_DestroySurface(result);
			return NULL;
		}
		if (result->format != format)
		{
			SDL_Surface* next = SDL_ConvertSurface(result, format);
			SDL_DestroySurface(result);
			result = next;
		}

		return result;
	}


	static bool loadShaderSPIRV(
		SDL_GPUDevice* device,
		SDL_GPUShader*& shader,
		const std::string& filename,
		SDL_GPUShaderStage stage,
		Uint32 sampler_count,
		Uint32 uniform_buffer_count,
		Uint32 storage_buffer_count,
		Uint32 storage_texture_count)
	{

		std::vector<Uint8> spirvCode = RenderUtil::loadBinaryFile(filename);
		if (spirvCode.empty()) {
			LogError(LOG_RENDER, "Failed to load shader %s", filename.c_str());
			return false;
		}

		SDL_GPUShaderCreateInfo createInfo = {};
		createInfo.num_samplers = sampler_count;
		createInfo.num_storage_buffers = storage_buffer_count;
		createInfo.num_storage_textures = storage_texture_count;
		createInfo.num_uniform_buffers = uniform_buffer_count;
		createInfo.props = 0;
		createInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
		createInfo.code = spirvCode.data();
		createInfo.code_size = spirvCode.size();
		createInfo.entrypoint = "main";
		createInfo.stage = stage;

		shader = SDL_CreateGPUShader(device, &createInfo);

		std::string stageName = " ";

		if (stage == SDL_GPU_SHADERSTAGE_VERTEX) {
			stageName = "vertex";
		}
		else if (stage == SDL_GPU_SHADERSTAGE_FRAGMENT) {
			stageName = "fragment";
		}

		if (shader == NULL) {
			LogError(LOG_RENDER, "Failed to create SDL_GPU %s shader form file ", stageName.c_str(), filename.c_str());

			SDL_ReleaseGPUShader(device, shader);
			return false;
		}
	}

	// 1x1 white pixel — lets lighting still show correctly
	// 0xFF000000 for black, or 0xFFFF00FF for obvious missing texture magenta.
    static SDL_GPUTexture* createColorTexture(SDL_GPUDevice* device, uint32_t color) {
       

        SDL_GPUTextureCreateInfo texInfo = {};
        texInfo.type = SDL_GPU_TEXTURETYPE_2D;
        texInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        texInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        texInfo.width = 1;
        texInfo.height = 1;
        texInfo.layer_count_or_depth = 1;
        texInfo.num_levels = 1;
        SDL_GPUTexture* tex = SDL_CreateGPUTexture(device, &texInfo);

        // Upload the pixel
        SDL_GPUTransferBufferCreateInfo transferBufferInfo = {};
		transferBufferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		transferBufferInfo.size = sizeof(uint32_t);
        SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferBufferInfo);

        void* mapped = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
        memcpy(mapped, &color, sizeof(color));
        SDL_UnmapGPUTransferBuffer(device, transferBuffer);

        SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(device);
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);

		SDL_GPUTextureTransferInfo textureTransferInfo = {
			.transfer_buffer = transferBuffer,
			.offset = 0,
		};

		SDL_GPUTextureRegion textureRegion = {
			.texture = tex,
			.w = 1,
			.h = 1,
			.d = 1

		};


		SDL_UploadToGPUTexture(copyPass, &textureTransferInfo, &textureRegion, false);
		SDL_EndGPUCopyPass(copyPass);
		SDL_SubmitGPUCommandBuffer(uploadCmdBuf);


		SDL_ReleaseGPUTransferBuffer(device, transferBuffer);

        return tex;
    }


	static bool uploadToTextureArray(SDL_GPUDevice* device, TextureArray & textureArray, SDL_Surface* imageData) {

		// Set up texture data
		const Uint32 imageSizeInBytes = imageData->w * imageData->h * 4;

		if (imageData->w != 1024 || imageData->h != 1024) {
			LogError(LOG_RENDER, "Texture must be 1024x1024, got %dx%d scaling the image", imageData->w, imageData->h);
			SDL_Surface* scaled = SDL_ScaleSurface(imageData, 1024, 1024, SDL_SCALEMODE_LINEAR);
		}

		if (textureArray.usedLayers >= textureArray.maxLayers) {
			LogError(LOG_RENDER, "TextureArray full!");
			return false;
		}
		
		SDL_GPUTextureRegion region{};
		region.texture = textureArray.textureArray;
		region.layer = textureArray.usedLayers ;  // which slot this texture occupies
		region.w = 1024;
		region.h = 1024;
		region.d = 1;

		// Set up buffer data
		SDL_GPUTransferBufferCreateInfo transferBufferInfo = {
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = imageSizeInBytes
		};
		SDL_GPUTransferBuffer* textureTransferBuffer = SDL_CreateGPUTransferBuffer(device, &transferBufferInfo);

		void* textureTransferPtr = SDL_MapGPUTransferBuffer(device, textureTransferBuffer, false);

		SDL_memcpy(textureTransferPtr, imageData->pixels, imageSizeInBytes);

		SDL_UnmapGPUTransferBuffer(device, textureTransferBuffer);

		// Upload the transfer data to the GPU resources
		SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(device);
		SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);



		SDL_GPUTextureTransferInfo textureTransferInfo = {
			.transfer_buffer = textureTransferBuffer,
			.offset = 0,
		};


		SDL_UploadToGPUTexture(copyPass, &textureTransferInfo, &region, false);
		SDL_EndGPUCopyPass(copyPass);
		SDL_SubmitGPUCommandBuffer(uploadCmdBuf);
	
		SDL_ReleaseGPUTransferBuffer(device, textureTransferBuffer);

		textureArray.usedLayers++;

		return true;

	}


	static bool saveSDLSurfaceToFile(fs::path filePath, TexHeader& header, SDL_Surface* imageData) {

		if (!imageData) {
			LogError(LOG_RENDER, "imageData for file %s is nullptr", filePath.c_str());
			return false;
		}

		std::filesystem::create_directories(filePath.parent_path());

		std::ofstream file(filePath, std::ios::binary);
		if (!file.is_open()) {
			LogError(LOG_RENDER, "Cannot open output file: %s", filePath.c_str());
			return false;
		}

		file.write(reinterpret_cast<const char*>(&header), sizeof(TexHeader));
		if (file.fail()) {
			LogError(LOG_RENDER, "Failed writing header to file: %s", filePath.c_str());
			return false;
		}

		if (SDL_MUSTLOCK(imageData)) SDL_LockSurface(imageData);

		file.write(reinterpret_cast<const char*>(imageData->pixels), header.pixelDataSize);
		if (file.fail()) {
			if (SDL_MUSTLOCK(imageData)) SDL_UnlockSurface(imageData);
			LogError(LOG_RENDER, "Failed writing pixel data to file: %s", filePath.c_str());
			return false;
		}

		if (SDL_MUSTLOCK(imageData)) SDL_UnlockSurface(imageData);

		return true;
	}

	static bool saveTexToFile(const fs::path& destFilePath, const TexHeader& header, const uint8_t* pixels)
	{
		if (!pixels) {
			LogError(LOG_RENDER, "pixels for file %s is nullptr", destFilePath.c_str());
			return false;
		}

		std::filesystem::create_directories(destFilePath.parent_path());
		std::ofstream file(destFilePath, std::ios::binary);
		if (!file.is_open()) {
			LogError(LOG_RENDER, "Cannot open output file: %s", destFilePath.c_str());
			return false;
		}

		file.write(reinterpret_cast<const char*>(&header), sizeof(TexHeader));
		if (file.fail()) {
			LogError(LOG_RENDER, "Failed writing header to file: %s", destFilePath.c_str());
			return false;
		}

		file.write(reinterpret_cast<const char*>(pixels), header.pixelDataSize);
		if (file.fail()) {
			LogError(LOG_RENDER, "Failed writing pixel data to file: %s", destFilePath.c_str());
			return false;
		}

		return true;
	}

	static bool loadTexFromFile(const fs::path& srcFilePath, TexHeader& header, std::vector<uint8_t>& pixels)
	{
		std::ifstream file(srcFilePath, std::ios::binary);
		if (!file.is_open()) {
			LogError(LOG_RENDER, "Cannot open texture file: %s", srcFilePath.c_str());
			return false;
		}

		file.read(reinterpret_cast<char*>(&header), sizeof(TexHeader));
		if (file.fail()) {
			LogError(LOG_RENDER, "Failed reading texture header from: %s", srcFilePath.c_str());
			return false;
		}

		if (header.magic != 0x544558) {
			LogError(LOG_RENDER, "Invalid texture magic number in file: %s", srcFilePath.c_str());
			return false;
		}

		if (header.pixelDataSize == 0 || header.width <= 0 || header.height <= 0) {
			LogError(LOG_RENDER, "Invalid texture dimensions in file: %s", srcFilePath.c_str());
			return false;
		}

		if (header.pitch < header.width) {
			LogError(LOG_RENDER, "Invalid texture pitch %d for width %d in file: %s",
				header.pitch, header.width, srcFilePath.c_str());
			return false;
		}

		if (header.pixelDataSize != static_cast<uint32_t>(header.pitch * header.height)) {
			LogError(LOG_RENDER, "Texture pixelDataSize %u doesn't match pitch*height %u in file: %s",
				header.pixelDataSize, header.pitch * header.height, srcFilePath.c_str());
			return false;
		}

		pixels.resize(header.pixelDataSize);
		file.read(reinterpret_cast<char*>(pixels.data()), header.pixelDataSize);
		if (file.fail()) {
			LogError(LOG_RENDER, "Failed reading pixel data from: %s", srcFilePath.c_str());
			return false;
		}

		return true;
	}

	static SDL_Surface* createSurfaceFromPixels(const TexHeader& header, std::vector<uint8_t>& pixels)
	{
		SDL_Surface* surface = SDL_CreateSurfaceFrom(
			header.width,
			header.height,
			static_cast<SDL_PixelFormat>(header.format),
			pixels.data(),
			header.pitch
		);

		if (!surface) {
			LogError(LOG_RENDER, "SDL_CreateSurfaceFrom failed: %s", SDL_GetError());
			return nullptr;
		}

		return surface;
	}

	static bool saveMaterialDataToFile(fs::path destFilePath, MaterialData & materialData) {

		std::filesystem::create_directories(destFilePath.parent_path());
		std::ofstream file(destFilePath, std::ios::binary);
		if (!file.is_open()) {
			LogError(LOG_RENDER, "Cannot open output file: %s", destFilePath.c_str());
			return false;
		}

		file.write(reinterpret_cast<const char*>(&materialData), sizeof(materialData)); 
		if (file.fail()) {
			LogError(LOG_RENDER, "Failed writing materialDdata to file: %s", destFilePath.c_str());
			return false;
		}

		return true;
	}

	static bool loadMaterialDataFromFile(const fs::path& srcFilePath, MaterialData& materialData)
	{
		std::ifstream file(srcFilePath, std::ios::binary);
		if (!file.is_open()) {
			LogError(LOG_RENDER, "Cannot open material file: %s", srcFilePath.c_str());
			return false;
		}

		// --- Validate file size before reading ---
		file.seekg(0, std::ios::end);
		const std::streamsize fileSize = file.tellg();
		file.seekg(0, std::ios::beg);

		if (fileSize != sizeof(MaterialData)) {
			LogError(LOG_RENDER, "Material file size mismatch (expected %zu, got %lld): %s",
				sizeof(MaterialData), fileSize, srcFilePath.c_str());
			return false;
		}

		file.read(reinterpret_cast<char*>(&materialData), sizeof(MaterialData));
		if (file.fail()) {
			LogError(LOG_RENDER, "Failed reading material data from: %s", srcFilePath.c_str());
			return false;
		}

		return true;
	}

	static bool saveMeshToFile(const fs::path& destFilePath, const Mesh& mesh ,const MeshHeader & header)
	{
		if (mesh.vertices.empty()) {
			LogError(LOG_RENDER, "Mesh has no vertices for file %s", destFilePath.c_str());
			return false;
		}

		std::filesystem::create_directories(destFilePath.parent_path());
		std::ofstream file(destFilePath, std::ios::binary);
		if (!file.is_open()) {
			LogError(LOG_RENDER, "Cannot open output file: %s", destFilePath.c_str());
			return false;
		}



		// --- Write header ---
		file.write(reinterpret_cast<const char*>(&header), sizeof(MeshHeader));
		if (file.fail()) {
			LogError(LOG_RENDER, "Failed writing mesh header to: %s", destFilePath.c_str());
			return false;
		}

		// --- Write submesh descriptors (only valid ones) ---
		for (int i = 0; i < mesh.subMeshCount; i++) {

			const SubMesh& sub = mesh.subMeshes[i];
			file.write(reinterpret_cast<const char*>(&sub), sizeof(SubMesh));
			if (file.fail()) {
				LogError(LOG_RENDER, "Failed writing submesh data to: %s", destFilePath.c_str());
				return false;
			}
		}

		// --- Write vertex buffer ---
		const size_t vertexDataSize = mesh.vertices.size() * sizeof(Vertex);
		file.write(reinterpret_cast<const char*>(mesh.vertices.data()), vertexDataSize);
		if (file.fail()) {
			LogError(LOG_RENDER, "Failed writing vertex data to: %s", destFilePath.c_str());
			return false;
		}

		// --- Write index buffer ---
		if (!mesh.indices.empty()) {
			const size_t indexDataSize = mesh.indices.size() * sizeof(uint32_t);
			file.write(reinterpret_cast<const char*>(mesh.indices.data()), indexDataSize);
			if (file.fail()) {
				LogError(LOG_RENDER, "Failed writing index data to: %s", destFilePath.c_str());
				return false;
			}
		}

		return true;
	}

	static bool loadMeshFromFile(const fs::path& srcFilePath, Mesh& mesh, MeshHeader& header)
	{
		std::ifstream file(srcFilePath, std::ios::binary);
		if (!file.is_open()) {
			LogError(LOG_RENDER, "Cannot open mesh file: %s", srcFilePath.c_str());
			return false;
		}

		// --- Read and validate header ---
		file.read(reinterpret_cast<char*>(&header), sizeof(MeshHeader));
		if (file.fail()) {
			LogError(LOG_RENDER, "Failed reading mesh header from: %s", srcFilePath.c_str());
			return false;
		}

		if (header.magic != 0x4D455348) {
			LogError(LOG_RENDER, "Invalid mesh magic number in file: %s", srcFilePath.c_str());
			return false;
		}

		if (header.version != 1) {
			LogError(LOG_RENDER, "Unsupported mesh version %u in file: %s", header.version, srcFilePath.c_str());
			return false;
		}

		if (header.vertexStride != sizeof(Vertex)) {
			LogError(LOG_RENDER, "Vertex stride mismatch (expected %zu, got %u) in file: %s",
				sizeof(Vertex), header.vertexStride, srcFilePath.c_str());
			return false;
		}

		if (header.vertexCount == 0) {
			LogError(LOG_RENDER, "Mesh has no vertices in file: %s", srcFilePath.c_str());
			return false;
		}

		if (header.subMeshCount > mesh.subMeshes.size()) {
			LogError(LOG_RENDER, "SubMesh count %u exceeds max capacity %zu in file: %s",
				header.subMeshCount, mesh.subMeshes.size(), srcFilePath.c_str());
			return false;
		}

		//Set size
		mesh.size = header.size;

		// --- Read submesh descriptors ---
		mesh.subMeshCount = header.subMeshCount;
		for (uint32_t i = 0; i < header.subMeshCount; i++) {
			file.read(reinterpret_cast<char*>(&mesh.subMeshes[i]), sizeof(SubMesh));
			if (file.fail()) {
				LogError(LOG_RENDER, "Failed reading submesh %u from: %s", i, srcFilePath.c_str());
				return false;
			}
		}

		// --- Read vertex buffer ---
		mesh.vertices.resize(header.vertexCount);
		const size_t vertexDataSize = header.vertexCount * sizeof(Vertex);
		file.read(reinterpret_cast<char*>(mesh.vertices.data()), vertexDataSize);
		if (file.fail()) {
			LogError(LOG_RENDER, "Failed reading vertex data from: %s", srcFilePath.c_str());
			return false;
		}

		// --- Read index buffer (optional) ---
		if (header.indexCount > 0) {
			mesh.indices.resize(header.indexCount);
			const size_t indexDataSize = header.indexCount * sizeof(uint32_t);
			file.read(reinterpret_cast<char*>(mesh.indices.data()), indexDataSize);
			if (file.fail()) {
				LogError(LOG_RENDER, "Failed reading index data from: %s", srcFilePath.c_str());
				return false;
			}
		}

		return true;
	}

	static bool saveSceneDataFile(const fs::path& destFilePath, const SceneHeader& sceneHeader,
		const std::vector<SceneNodeData>& nodeData)
	{
		std::error_code ec;
		std::filesystem::create_directories(destFilePath.parent_path(), ec);
		if (ec) {
			LogError(LOG_RENDER, "Failed to create directories for: %s (%s)",
				destFilePath.c_str(), ec.message().c_str());
			return false;
		}

		std::ofstream file(destFilePath, std::ios::binary);
		if (!file.is_open()) {
			LogError(LOG_RENDER, "Cannot open output file: %s", destFilePath.c_str());
			return false;
		}

		// Stamp nodesNum from the actual vector size before writing
		SceneHeader headerToWrite = sceneHeader;
		headerToWrite.nodesNum = static_cast<uint32_t>(nodeData.size());

		file.write(reinterpret_cast<const char*>(&headerToWrite), sizeof(SceneHeader));
		if (file.fail()) {
			LogError(LOG_RENDER, "Failed writing SceneHeader to file: %s", destFilePath.c_str());
			return false;
		}

		for (const SceneNodeData& sceneNode : nodeData) {
			// Serialize name as length-prefixed string
			const uint32_t nameLen = static_cast<uint32_t>(sceneNode.name.size());
			file.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
			file.write(sceneNode.name.data(), nameLen);

			file.write(reinterpret_cast<const char*>(&sceneNode.meshID), sizeof(sceneNode.meshID));
			file.write(reinterpret_cast<const char*>(&sceneNode.transform), sizeof(sceneNode.transform));

			if (file.fail()) {
				LogError(LOG_RENDER, "Failed writing SceneNodeData to file: %s", destFilePath.c_str());
				return false;
			}
		}

		return true;
	}

	static bool loadSceneDataFile(const fs::path& srcFilePath, SceneHeader& sceneHeader,
		std::vector<SceneNodeData>& nodeData)
	{
		std::ifstream file(srcFilePath, std::ios::binary);
		if (!file.is_open()) {
			LogError(LOG_RENDER, "Cannot open scene file: %s", srcFilePath.c_str());
			return false;
		}

		// Read and validate header first
		file.read(reinterpret_cast<char*>(&sceneHeader), sizeof(SceneHeader));
		if (file.fail()) {
			LogError(LOG_RENDER, "Failed reading SceneHeader from: %s", srcFilePath.c_str());
			return false;
		}

		if (sceneHeader.magic != 0x5343454e) {   // 'SCEN' — fits in uint32_t
			LogError(LOG_RENDER, "Invalid scene header magic number in file: %s", srcFilePath.c_str());
			return false;
		}

		// Validate remaining file size against the node count in the header.
		// We can't know exact size without reading the variable-length name strings,
		// so just check there's at least some data left when nodesNum > 0.
		if (sceneHeader.nodesNum > 0) {
			const std::streampos currentPos = file.tellg();
			file.seekg(0, std::ios::end);
			const std::streamsize remaining = static_cast<std::streamsize>(file.tellg() - currentPos);
			file.seekg(currentPos);

			if (remaining <= 0) {
				LogError(LOG_RENDER, "Scene file has no node data despite nodesNum=%u: %s",
					sceneHeader.nodesNum, srcFilePath.c_str());
				return false;
			}
		}

		nodeData.resize(sceneHeader.nodesNum);

		for (SceneNodeData& sceneNode : nodeData) {
			// Read length-prefixed name
			uint32_t nameLen = 0;
			file.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
			if (file.fail()) {
				LogError(LOG_RENDER, "Failed reading name length from: %s", srcFilePath.c_str());
				return false;
			}

			// Sanity-cap name length to guard against corrupt files
			constexpr uint32_t kMaxNameLen = 4096;
			if (nameLen > kMaxNameLen) {
				LogError(LOG_RENDER, "Implausible name length %u in file: %s", nameLen, srcFilePath.c_str());
				return false;
			}

			sceneNode.name.resize(nameLen);
			file.read(sceneNode.name.data(), nameLen);

			file.read(reinterpret_cast<char*>(&sceneNode.meshID), sizeof(sceneNode.meshID));
			file.read(reinterpret_cast<char*>(&sceneNode.transform), sizeof(sceneNode.transform));

			if (file.fail()) {
				LogError(LOG_RENDER, "Failed reading SceneNodeData from: %s", srcFilePath.c_str());
				return false;
			}
		}

		return true;
	}

};