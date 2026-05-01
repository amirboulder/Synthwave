module;

#include <iostream>
#include <vector>
#include <fstream>
#include <filesystem>

//Slang
#include <slang.h>
#include "slang-com-ptr.h"
#include "slang-com-helper.h"

#include "rapidjson/prettywriter.h"
#include "rapidjson/filewritestream.h"

namespace fs = std::filesystem;

export module ShaderCompiler;

import Logger;


void diagnoseIfNeeded(slang::IBlob* diagnosticsBlob)
{
	if (diagnosticsBlob != nullptr)
	{
		LogError(LOG_RENDER, (const char*)diagnosticsBlob->getBufferPointer());
	}
}

/// <summary>
/// Generates SPRIV Graphics and compute shaders as well as a shader metadata.json file from Slang source files.
/// Vulkan uses SPIRV as its Intermediate Representation, we use the following extensions for SPRIV files 
/// Vertex shader (.vert.spv) ,fragment shader (.frag.spv), compute shader (.comp.spv)
/// </summary>
export class ShaderCompiler {

public:

	//TODO get these from a config file
	const char* spirvVersion = "spirv_1_3";
	const char* shaderOutputFolder = "shaders/compiled/";

	ShaderCompiler() {

	}


	int compileAllShaders(const std::vector<fs::path> srcPaths) {

		// Create the Global Session
		Slang::ComPtr<slang::IGlobalSession> slangGlobalSession;
		if (0 != slang::createGlobalSession(slangGlobalSession.writeRef()))
		{
			LogError(LOG_RENDER, "Slang createGlobalSession failed ");
			return -1;
		}

		LogVerbose(LOG_RENDER, "Created Slang Global Session");

		// Configure the Compilation Session
		slang::SessionDesc sessionDesc = {};

		// TargetDesc specifies the output format and version
		slang::TargetDesc targetDesc = {};
		targetDesc.format = SLANG_SPIRV;              // Output SPIRV bytecode
		targetDesc.profile = slangGlobalSession->findProfile(spirvVersion);
		targetDesc.flags = 0;

		// Link the target to the session
		sessionDesc.targets = &targetDesc;
		sessionDesc.targetCount = 1;
		sessionDesc.compilerOptionEntryCount = 0;

		// Create the actual compilation session
		Slang::ComPtr<slang::ISession> session;
		if (0 != slangGlobalSession->createSession(sessionDesc, session.writeRef()))
		{
			LogError(LOG_RENDER, "Slang createSession failed");
			return -1;
		}

		LogVerbose(LOG_RENDER, "Created Slang compilation session");

		for (const fs::path& srcPath : srcPaths) {

			Slang::ComPtr<slang::IBlob> diagnosticsBlob;
			slang::IModule* slangModule = session->loadModule(srcPath.string().c_str(), diagnosticsBlob.writeRef());
			diagnoseIfNeeded(diagnosticsBlob);
			if (!slangModule)
			{
				LogError(LOG_RENDER, "Failed creating Slang Module for file : %s ", srcPath.string().c_str());
				continue;

			}
			LogVerbose(LOG_RENDER, "Created Slang Module");


			// Find Entry Points
			// An entry point is a specific function that serves as the "main" for a shader stage
			Slang::ComPtr<slang::IEntryPoint> vertexEntryPoint;
			Slang::ComPtr<slang::IEntryPoint> fragmentEntryPoint;
			Slang::ComPtr<slang::IEntryPoint> computeEntryPoint;

			SlangResult vertexMainFound = slangModule->findEntryPointByName("vertexMain", vertexEntryPoint.writeRef());
			SlangResult fragmentMainFound = slangModule->findEntryPointByName("fragmentMain", fragmentEntryPoint.writeRef());
			SlangResult computeMainFound = slangModule->findEntryPointByName("computeMain", computeEntryPoint.writeRef());


			if (0 == vertexMainFound && 0 == fragmentMainFound) {

				compileGraphicsShader(session, slangModule, srcPath, vertexEntryPoint, fragmentEntryPoint);
			}
			else if (0 == computeMainFound) {

				compileComputeShader(session, slangModule, srcPath, computeEntryPoint);
			}
			else {

				/*
				* This is not entirely accurate as a shader file may just contain a fragment or a vertex shader,
				*  but we don't support that!!!
				*/
				LogError(LOG_RENDER, "findEntryPointByName failed for vertexMain && fragmentMain, and computeMain in file : %s ", srcPath.string().c_str());
			}
		}

		return 0;
	}

	int compileGraphicsShader(
		Slang::ComPtr<slang::ISession> session,
		slang::IModule* slangModule,
		const fs::path& srcPath,
		Slang::ComPtr<slang::IEntryPoint> vertexEntryPoint,
		Slang::ComPtr<slang::IEntryPoint> fragmentEntryPoint)
	{

		// Compose the Program
		// Combine the module and entry point into a composite program
		// This creates the final executable shader program
		Slang::ComPtr<slang::IComponentType> composedProgram;
		slang::IComponentType* components[] = { slangModule, vertexEntryPoint,fragmentEntryPoint };
		int numberOfComponents = 3;
		if (0 != session->createCompositeComponentType(components, numberOfComponents, composedProgram.writeRef()))
		{
			LogError(LOG_RENDER, "Slang failed to createCompositeComponentType in file : %s ", srcPath.string().c_str());
			return -1;
		}

		// Generate SPIRV Bytecode
		// Extract the compiled SPIRV code from the composed program
		Slang::ComPtr<slang::IBlob> spirvVertexShaderCode;
		Slang::ComPtr<slang::IBlob> diagnosticsVertexShader;

		Slang::ComPtr<slang::IBlob> spirvFragmentShaderCode;
		Slang::ComPtr<slang::IBlob> diagnostics2;

		SlangResult resultVertex = composedProgram->getEntryPointCode(
			0,                      // entry point index
			0,                      // target index (we only have one target)
			spirvVertexShaderCode.writeRef(),
			diagnosticsVertexShader.writeRef()
		);

		SlangResult resultFragment = composedProgram->getEntryPointCode(
			1,                      // entry point index
			0,                      // target index (we only have one target)
			spirvFragmentShaderCode.writeRef(),
			diagnostics2.writeRef()
		);

		// Check for code generation errors
		if (diagnosticsVertexShader && diagnosticsVertexShader->getBufferSize() > 0) {
			LogWarn(LOG_RENDER, "Code generation diagnostics:\n%s\n", (const char*)diagnosticsVertexShader->getBufferPointer());
		}

		if (SLANG_FAILED(resultVertex) || SLANG_FAILED(resultFragment)) {
			LogError(LOG_RENDER, "Failed to generate SPIRV code from file : %s", srcPath.string().c_str());
			return -1;
		}

		if (!spirvVertexShaderCode || spirvVertexShaderCode->getBufferSize() == 0) {
			LogError(LOG_RENDER, "No SPIRV Vertex shader code generated from file : %s", srcPath.string().c_str());
			return -1;
		}

		if (!spirvFragmentShaderCode || spirvFragmentShaderCode->getBufferSize() == 0) {
			LogError(LOG_RENDER, "No SPIRV fragment shader code generated from file : %s", srcPath.string().c_str());
			return -1;
		}

		// Output the SPIRV Bytecode
		LogTrace(LOG_RENDER, "Successfully generated SPIRV vertex shader code: %zu bytes", spirvVertexShaderCode->getBufferSize());
		LogTrace(LOG_RENDER, "Successfully generated SPIRV fragment shader code: %zu bytes", spirvFragmentShaderCode->getBufferSize());

		// Ensure the directory exists
		std::filesystem::path directory(shaderOutputFolder);

		if (!std::filesystem::exists(directory)) {

			LogInfo(LOG_RENDER, "Directory %s does not exist creating directory", directory.c_str());

			std::filesystem::create_directories(directory);
		}

		fs::path outputPathVS = directory / (srcPath.stem().string() + ".vert.spv");
		fs::path outputPathFS = directory / (srcPath.stem().string() + ".frag.spv");

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

		LogVerbose(LOG_RENDER, "Successfully created SPIRV file :  %s", outputPathVS.string().c_str());
		LogVerbose(LOG_RENDER, "Successfully created SPIRV file :  %s", outputPathFS.string().c_str());


		//Get Shader Reflection Data
		slang::ShaderReflection* reflection = composedProgram->getLayout(0);
		if (!reflection) {
			LogError(LOG_RENDER, "Failed to get shader reflection data for file :  %s", srcPath.string().c_str());
			return -1;
		}

		//SDL_GPU docs on Shader resource bindings
		// https://wiki.libsdl.org/SDL3/SDL_CreateGPUShader

		//printReflectionBlob(reflection);

		std::string outputPathJSON = shaderOutputFolder;
		outputPathJSON += srcPath.stem().string();
		outputPathJSON += ".json";

		writeReflectionToFile(reflection, outputPathJSON);

	}

	//TODO finish compute shader reflection
	int compileComputeShader(Slang::ComPtr<slang::ISession> session,
		slang::IModule* slangModule,
		const fs::path& srcPath,
		Slang::ComPtr<slang::IEntryPoint> computeEntryPoint) {

		// Compose the Program
		// Combine the module and entry point into a composite program
		// This creates the final executable shader program
		Slang::ComPtr<slang::IComponentType> composedProgram;
		slang::IComponentType* components[] = { slangModule, computeEntryPoint };
		int numberOfComponents = 2;
		if (0 != session->createCompositeComponentType(components, numberOfComponents, composedProgram.writeRef()))
		{
			LogError(LOG_RENDER, "Slang failed to createCompositeComponentType in file : %s ", srcPath.string().c_str());
			return -1;
		}

		// Generate SPIRV Bytecode
		// Extract the compiled SPIRV code from the composed program
		Slang::ComPtr<slang::IBlob> spirvComputeShaderCode;
		Slang::ComPtr<slang::IBlob> diagnosticsComputeShader;

		SlangResult composedProgramResult = composedProgram->getEntryPointCode(
			0,                      // entry point index (we only have one)
			0,                      // target index (we only have one target)
			spirvComputeShaderCode.writeRef(),
			diagnosticsComputeShader.writeRef()
		);


		// Check for code generation errors
		if (diagnosticsComputeShader && diagnosticsComputeShader->getBufferSize() > 0) {
			LogWarn(LOG_RENDER, "Code generation diagnostics:\n%s\n", (const char*)diagnosticsComputeShader->getBufferPointer());
		}

		if (SLANG_FAILED(composedProgramResult)) {
			LogError(LOG_RENDER, "Failed to generate SPIRV code from file : %s", srcPath.string().c_str());
			return -1;
		}

		if (!spirvComputeShaderCode || spirvComputeShaderCode->getBufferSize() == 0) {
			LogError(LOG_RENDER, "No SPIRV compute shader code generated from file : %s", srcPath.string().c_str());
			return -1;
		}

		// Output the SPIRV Bytecode
		LogTrace(LOG_RENDER, "Successfully generated SPIRV compute shader code: %zu bytes", spirvComputeShaderCode->getBufferSize());

		// Ensure the directory exists
		std::filesystem::path directory(shaderOutputFolder);

		if (!std::filesystem::exists(directory)) {

			LogInfo(LOG_RENDER, "Directory %s does not exist creating directory", directory.c_str());

			std::filesystem::create_directories(directory);
		}

		fs::path outputPathCS = directory / (srcPath.stem().string() + ".comp.spv");

		std::ofstream outFileCS(outputPathCS, std::ios::out | std::ios::binary);


		// Write SPIRV binary data to files
		outFileCS.write(
			static_cast<const char*>(spirvComputeShaderCode->getBufferPointer()),
			spirvComputeShaderCode->getBufferSize()
		);

		LogVerbose(LOG_RENDER, "Successfully created SPIRV file :  %s", outputPathCS.string().c_str());


		//Write shader reflection data to a file
		slang::ShaderReflection* reflection = composedProgram->getLayout();
		if (!reflection) {
			LogError(LOG_RENDER, "Failed to get shader reflection data");
			return -1;
		}

		std::string outputPathJSON = shaderOutputFolder;
		outputPathJSON += srcPath.stem().string();
		outputPathJSON += ".json";

		writeReflectionToFile(reflection, outputPathJSON);
	}

	void printReflectionBlob(slang::ShaderReflection* reflection) {

		// Create a blob pointer to receive the JSON data
		ISlangBlob* jsonBlob = nullptr;

		// Convert reflection to JSON
		SlangResult result = reflection->toJson(&jsonBlob);

		if (SLANG_SUCCEEDED(result) && jsonBlob)
		{
			// Get the JSON string from the blob
			const char* jsonStr = (const char*)jsonBlob->getBufferPointer();
			size_t jsonSize = jsonBlob->getBufferSize();

			// Print to console
			std::cout << std::string(jsonStr, jsonSize) << std::endl;

			// Release the blob when done
			jsonBlob->release();
		}
		else
		{
			std::cerr << "Failed to convert reflection to JSON" << std::endl;
		}
	}

	void writeReflectionToFile(slang::ShaderReflection* reflection, std::string filename) {
		ISlangBlob* jsonBlob = nullptr;
		SlangResult result = reflection->toJson(&jsonBlob);

		if (SLANG_SUCCEEDED(result) && jsonBlob) {
			const char* jsonStr = (const char*)jsonBlob->getBufferPointer();
			size_t jsonSize = jsonBlob->getBufferSize();

			std::ofstream outFile(filename);
			if (!outFile.is_open()) {
				LogError(LOG_RENDER, "Failed to open file %s for writing", filename.c_str());
				jsonBlob->release();
				return;
			}

			outFile.write(jsonStr, jsonSize);
			jsonBlob->release();
			LogDebug(LOG_RENDER, "Wrote reflection data to %s", filename.c_str());
		}
		else {
			LogError(LOG_RENDER, "Failed to write %s reflection to JSON", filename.c_str());
		}
	}

};



