#pragma once

#include <SDL3/SDL.h>

#include<glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <fstream>
#include <filesystem>

using std::cout;
using std::string;

#include "slang-com-ptr.h"
#include <slang.h>
#include "slang-com-helper.h"


#define RETURN_ON_FAIL(x) \
    {                     \
        auto _res = x;    \
        if (_res != 0)    \
        {                 \
            return -1;    \
        }                 \
    }


void diagnoseIfNeeded(slang::IBlob* diagnosticsBlob)
{
	if (diagnosticsBlob != nullptr)
	{
		std::cout << (const char*)diagnosticsBlob->getBufferPointer() << std::endl;
	}
}



namespace shader {

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

	SDL_GPUShader* load_VertexShader(
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
			return nullptr;
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

		return SDL_CreateGPUShader(device, &createinfo);
	}

	SDL_GPUShader* load_FragmentShader(
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
			std::cerr << "Failed to load fragment shader: " << filename << std::endl;
			return nullptr;
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

		return SDL_CreateGPUShader(device, &createinfo);
	}


	int generateSpirvShaders(const char* shaderPath, const char* outputPathVS, const char* outputPathFS) {

		// STEP 1: Create the Global Session
		Slang::ComPtr<slang::IGlobalSession> slangGlobalSession;
		RETURN_ON_FAIL(slang::createGlobalSession(slangGlobalSession.writeRef()));
		printf("Created slangGlobalSession \n");


		// STEP 2: Configure the Compilation Session
		slang::SessionDesc sessionDesc = {};

		// TargetDesc specifies the output format and version
		slang::TargetDesc targetDesc = {};
		targetDesc.format = SLANG_SPIRV;              // Output SPIRV bytecode (for Vulkan)
		targetDesc.profile = slangGlobalSession->findProfile("spirv_1_3");
		targetDesc.flags = 0;



		// Link the target to the session
		sessionDesc.targets = &targetDesc;
		sessionDesc.targetCount = 1;
		sessionDesc.compilerOptionEntryCount = 0;
		// Create the actual compilation session
		Slang::ComPtr<slang::ISession> session;
		RETURN_ON_FAIL(slangGlobalSession->createSession(sessionDesc, session.writeRef()));

		printf("Created compilation session \n");



		Slang::ComPtr<slang::IBlob> diagnosticsBlob;
		//const char* shaderPath = "shaders/slang/shaders.slang";
		slang::IModule* slangModule =
			session->loadModule(shaderPath, diagnosticsBlob.writeRef());
		diagnoseIfNeeded(diagnosticsBlob);
		if (!slangModule)
			return SLANG_FAIL;

		printf("Created module \n");



		// STEP 4: Find Entry Points
		// An entry point is a specific function that serves as the "main" for a shader stage
		Slang::ComPtr<slang::IEntryPoint> vertexEntryPoint;
		RETURN_ON_FAIL(
			slangModule->findEntryPointByName("vertexMain", vertexEntryPoint.writeRef()));
		//
		Slang::ComPtr<slang::IEntryPoint> fragmentEntryPoint;
		RETURN_ON_FAIL(
			slangModule->findEntryPointByName("fragmentMain", fragmentEntryPoint.writeRef()));


		// STEP 5: Compose the Program
		// Combine the module and entry point into a composite program
		// This creates the final executable shader program
		Slang::ComPtr<slang::IComponentType> composedProgram;
		slang::IComponentType* components[] = { slangModule, vertexEntryPoint,fragmentEntryPoint };
		RETURN_ON_FAIL(session->createCompositeComponentType(
			components,
			3,  // number of components
			composedProgram.writeRef()
		));


		// STEP 6: Generate SPIRV Bytecode
		// Extract the compiled SPIRV code from the composed program
		Slang::ComPtr<slang::IBlob> spirvVertexShaderCode;
		Slang::ComPtr<slang::IBlob> diagnosticsVertexShader;

		Slang::ComPtr<slang::IBlob> spirvFragmentShaderCode;
		Slang::ComPtr<slang::IBlob> diagnostics2;

		SlangResult resultVertex = composedProgram->getEntryPointCode(
			0,                      // entry point index (we only have one)
			0,                      // target index (we only have one target)
			spirvVertexShaderCode.writeRef(),
			diagnosticsVertexShader.writeRef()
		);

		SlangResult resultFragment = composedProgram->getEntryPointCode(
			1,                      // entry point index (we only have one)
			0,                      // target index (we only have one target)
			spirvFragmentShaderCode.writeRef(),
			diagnostics2.writeRef()
		);

		// Check for code generation errors
		if (diagnosticsVertexShader && diagnosticsVertexShader->getBufferSize() > 0) {
			printf("Code generation diagnostics:\n%s\n",
				(const char*)diagnosticsVertexShader->getBufferPointer());
		}

		if (SLANG_FAILED(resultVertex) || SLANG_FAILED(resultFragment)) {
			printf("Failed to generate SPIRV code\n");
			return -1;
		}

		if (!spirvFragmentShaderCode || spirvFragmentShaderCode->getBufferSize() == 0) {
			printf("No SPIRV fragment shader code generated\n");
			return -1;
		}

		if (!spirvVertexShaderCode || spirvVertexShaderCode->getBufferSize() == 0) {
			printf("No SPIRV code generated\n");
			return -1;
		}

		// STEP 7: Output the SPIRV Bytecode
		printf("Successfully generated SPIRV code: %zu bytes\n", spirvVertexShaderCode->getBufferSize());
		printf("Successfully generated SPIRV fragment shader code: %zu bytes\n", spirvFragmentShaderCode->getBufferSize());

		// Ensure the directory exists
		std::filesystem::path fullPath(outputPathVS);
		std::filesystem::path directory = fullPath.parent_path();

		if (!std::filesystem::exists(directory)) {

			printf("\033[31mDirectory %s does not exist creating directory\033[0m\n", directory.c_str());

			std::filesystem::create_directories(directory);
		}


		std::ofstream outFileVS(outputPathVS, std::ios::out | std::ios::binary);
		std::ofstream outFileFS(outputPathFS, std::ios::out | std::ios::binary);

	
		// Write SPIRV binary data to files
		outFileVS.write(
			static_cast<const char*>(spirvVertexShaderCode->getBufferPointer()),
			spirvVertexShaderCode->getBufferSize()
		);

		outFileFS.write(
			static_cast<const char*>(spirvFragmentShaderCode->getBufferPointer()),
			spirvFragmentShaderCode->getBufferSize()
		);


		printf("Successfully created spriv files!\n");

		return 0;
	}

}