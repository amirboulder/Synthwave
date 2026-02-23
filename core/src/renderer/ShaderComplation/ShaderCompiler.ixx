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
/// Pipelines differ in terms of their input and output.
/// This enum is used determinate which Pipeline::createPipeline should be used for each pipeline
/// </summary>
export enum class PipelineType {
	NONE,
	Vertex,
	PhysicsDebug,
	LineVertex,
	EntityId,
};

struct VertexInputInfo {
	bool hasPosition = false;
	bool hasNormal = false;
	bool hasTexCoords = false;
	bool hasColor = false;
	int fieldCount = 0;
};

struct FragmentOutputInfo {
	slang::TypeReflection::ScalarType scalarType;
	int componentCount = 0;
};

export struct ShaderReflectionData {

	std::string outputFile;

	uint32_t numSamplers = 0;
	uint32_t numStorageBuffers = 0;
	uint32_t numStorageTextures = 0;
	uint32_t numUniformBuffers = 0;
	uint32_t numSampledTextures = 0; // remove


};

export struct ComputeShaderReflectionData {

	std::string outputFile;

	uint32_t numSamplers = 0;
	uint32_t numUniformBuffers = 0;

	uint32_t numReadOnlyStorageBuffers = 0;
	uint32_t numReadOnlyStorageTextures = 0;
	uint32_t numReadwriteStorageTextures = 0;
	uint32_t numReadwriteStorageBuffers = 0;

	uint32_t threadCountX = 0;
	uint32_t threadCountY = 0;
	uint32_t threadCountZ = 0;
};


/// <summary>
/// Generates SPRIV Graphics and compute shaders as well as a shader metadata.json file from Slang source files.
/// Vulkan uses SPIRV as its Intermediate Representation, we use the following extension for SPRIV files 
/// Vertex shader (.vert.spv) ,fragment shader (.vert.spv), compute shader (.comp.spv)
/// Metadata file is created using information from slang's shader reflection api.
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
				LogError(LOG_RENDER, "findEntryPointByName failed for vertexMain in file : %s ", srcPath.string().c_str());
				LogError(LOG_RENDER, "findEntryPointByName failed for fragmentMain in file : %s ", srcPath.string().c_str());
				LogError(LOG_RENDER, "findEntryPointByName failed for computeMain in file : %s ", srcPath.string().c_str());
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



		PipelineType pipelineType = identifyPipelineType(reflection);


		ShaderReflectionData vsData;
		ShaderReflectionData fsData;


		for (unsigned int i = 0; i < reflection->getParameterCount(); i++) {
			slang::VariableLayoutReflection* param = reflection->getParameterByIndex(i);

			// Get the binding set to determine which stage this resource belongs to
			unsigned int bindingSet = param->getBindingSpace();

			// Per SDL_GPU convention: sets 0,1 = vertex, sets 2,3 = fragment
			ShaderReflectionData& stageData = (bindingSet <= 1) ? vsData : fsData;

			collectReflectionData(param, stageData);
		}


		// Write JSON
		std::string outputPathJSON = shaderOutputFolder;
		outputPathJSON += srcPath.stem().string();
		outputPathJSON += ".json";

		FILE* fp = fopen(outputPathJSON.c_str(), "wb");
		if (!fp) {
			LogError(LOG_RENDER, "Failed to open reflection output file: %s", outputPathJSON.c_str());
			return -1;
		}

		char writeBuffer[65536];
		rapidjson::FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
		rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer(os);

		writer.StartObject();

		writer.Key("shader");
		writer.String(srcPath.filename().string().c_str());

		writer.Key("shaderType");
		writer.String("graphics");

		writer.Key("source");
		writer.String(srcPath.string().c_str());

		writer.Key("pipelineType");
		writer.Uint(static_cast<int>(pipelineType));

		writer.Key("vertex");
		writer.StartObject();

		writer.Key("numSamplers");
		writer.Uint(vsData.numSamplers);

		writer.Key("numStorageBuffers");
		writer.Uint(vsData.numStorageBuffers);

		writer.Key("numStorageTextures");
		writer.Uint(vsData.numStorageTextures);

		writer.Key("numUniformBuffers");
		writer.Uint(vsData.numUniformBuffers);

		writer.Key("outputFile");
		writer.String(outputPathVS.string().c_str());

		writer.EndObject();

		writer.Key("fragment");
		writer.StartObject();

		writer.Key("numSamplers");
		writer.Uint(fsData.numSamplers);

		writer.Key("numStorageBuffers");
		writer.Uint(fsData.numStorageBuffers);

		writer.Key("numStorageTextures");
		writer.Uint(fsData.numStorageTextures);

		writer.Key("numUniformBuffers");
		writer.Uint(fsData.numUniformBuffers);

		writer.Key("outputFile");
		writer.String(outputPathFS.string().c_str());

		writer.EndObject();

		writer.EndObject();

		fclose(fp);

		return 0;
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

		if (SLANG_FAILED(composedProgramResult) ) {
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

		//TODO
		//Write shader reflection data to a file
		slang::ShaderReflection* reflection = composedProgram->getLayout();
		if (!reflection) {
			LogError(LOG_RENDER, "Failed to get shader reflection data");
			return -1;
		}

		//writeReflectionBlobToFile(reflection);


		ComputeShaderReflectionData reflectionDataCS;


		for (unsigned int i = 0; i < reflection->getParameterCount(); i++) {
			slang::VariableLayoutReflection* param = reflection->getParameterByIndex(i);

			collectComputeReflectionData(param, reflectionDataCS);
		}

		slang::EntryPointReflection* reflectionEP0 = reflection->getEntryPointByIndex(0);

		SlangUInt threadGroupSize[3] = { 0, 0, 0 };
		reflectionEP0->getComputeThreadGroupSize(3, threadGroupSize);

		reflectionDataCS.threadCountX = static_cast<uint32_t>(threadGroupSize[0]);
		reflectionDataCS.threadCountY = static_cast<uint32_t>(threadGroupSize[1]);
		reflectionDataCS.threadCountZ = static_cast<uint32_t>(threadGroupSize[2]);

		// Build the output path: same location as the .spv files, e.g. "shaderName.slang.json"
		std::string outputPathJSON = shaderOutputFolder;
		outputPathJSON += srcPath.stem().string();
		outputPathJSON += ".json";

		FILE* fp = fopen(outputPathJSON.c_str(), "wb");
		if (!fp) {
			LogError(LOG_RENDER, "Failed to open reflection output file: %s", outputPathJSON.c_str());
			return -1;
		}

		char writeBuffer[65536];
		rapidjson::FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
		rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer(os);

		writer.StartObject();

		writer.Key("shader");
		writer.String(srcPath.filename().string().c_str());

		writer.Key("shaderType");
		writer.String("compute");

		writer.Key("source");
		writer.String(srcPath.string().c_str());

		writer.Key("compute");
		writer.StartObject();

		writer.Key("numSamplers");
		writer.Uint(reflectionDataCS.numSamplers);

		writer.Key("numUniformBuffers");
		writer.Uint(reflectionDataCS.numUniformBuffers);

		writer.Key("numReadOnlyStorageBuffers");
		writer.Uint(reflectionDataCS.numReadOnlyStorageBuffers);

		writer.Key("numReadOnlyStorageTextures");
		writer.Uint(reflectionDataCS.numReadOnlyStorageTextures);

		writer.Key("numReadwriteStorageBuffers");
		writer.Uint(reflectionDataCS.numReadwriteStorageBuffers);

		writer.Key("numReadwriteStorageTextures");
		writer.Uint(reflectionDataCS.numReadwriteStorageTextures);

		writer.Key("threadCountX");
		writer.Uint64(reflectionDataCS.threadCountX);

		writer.Key("threadCountY");
		writer.Uint64(reflectionDataCS.threadCountY);

		writer.Key("threadCountZ");
		writer.Uint64(reflectionDataCS.threadCountZ);

		writer.Key("outputFile");
		writer.String(outputPathCS.string().c_str());

		writer.EndObject();
		writer.EndObject();

		fclose(fp);

		LogVerbose(LOG_RENDER, "Successfully wrote reflection data to %s", outputPathJSON.c_str());


		return 0;
	}

	void collectReflectionData(slang::VariableLayoutReflection* param, ShaderReflectionData& data) {

		slang::TypeReflection::Kind kind = param->getType()->getKind();
		SlangResourceShape          shape = param->getTypeLayout()->getType()->getResourceShape();
		SlangResourceAccess access = param->getType()->getResourceAccess();

		switch (kind) {
		case slang::TypeReflection::Kind::SamplerState:  // kind 8
			data.numSamplers++;
			break;

		case slang::TypeReflection::Kind::ConstantBuffer: // kind 6
			data.numUniformBuffers++;
			break;

		case slang::TypeReflection::Kind::Resource:       // kind 7
			switch (shape) {
			case SLANG_TEXTURE_1D:
			case SLANG_TEXTURE_2D:
			case SLANG_TEXTURE_3D:
			case SLANG_TEXTURE_CUBE:
				data.numSampledTextures++;
				break;
			case SLANG_TEXTURE_2D_ARRAY: // just in case
				data.numSampledTextures++;
				break;
			case SLANG_STRUCTURED_BUFFER:
			case SLANG_BYTE_ADDRESS_BUFFER:
				data.numStorageBuffers++;
				break;
			default:
				LogWarn(LOG_RENDER, "Unhandled resource shape %d for parameter %s",
					shape, param->getName());
				break;
			}
			break;

		default:
			LogWarn(LOG_RENDER, "Unhandled resource kind %d for parameter %s",
				static_cast<int>(kind), param->getName());
			break;
		}


	}

	void collectComputeReflectionData(slang::VariableLayoutReflection* param, ComputeShaderReflectionData& data) {
		slang::TypeReflection::Kind kind = param->getType()->getKind();
		SlangResourceShape          shape = param->getTypeLayout()->getType()->getResourceShape();
		SlangResourceAccess access = param->getType()->getResourceAccess();

		std::string name = param->getName();

		switch (kind) {
		case slang::TypeReflection::Kind::SamplerState:  // kind 8
			data.numSamplers++;
			break;
		case slang::TypeReflection::Kind::ConstantBuffer: // kind 6
			data.numUniformBuffers++;
			break;
		case slang::TypeReflection::Kind::Resource:       // kind 7
			switch (shape) {
			case SLANG_TEXTURE_1D:
			case SLANG_TEXTURE_2D:
			case SLANG_TEXTURE_3D:
			case SLANG_TEXTURE_CUBE:
				
				if (access == SLANG_RESOURCE_ACCESS_READ_WRITE) {
					data.numReadwriteStorageTextures++;
				}
				else if (access == SLANG_RESOURCE_ACCESS_READ) {
					data.numReadOnlyStorageTextures++;
				}
				
				break;
			case SLANG_STRUCTURED_BUFFER:
			case SLANG_BYTE_ADDRESS_BUFFER:
				switch (access) { 
				case SLANG_RESOURCE_ACCESS_READ:
					data.numReadOnlyStorageBuffers++;
					break;
				case SLANG_RESOURCE_ACCESS_READ_WRITE:
					data.numReadwriteStorageBuffers++;
					break;
				}
				break;
			default:
				LogWarn(LOG_RENDER, "Unhandled resource shape %d for parameter %s",
					shape, param->getName());
				break;
			}
			break;
		default:
			LogWarn(LOG_RENDER, "Unhandled resource kind %d for parameter %s",
				static_cast<int>(kind), param->getName());
			break;
		}
	}


	VertexInputInfo getVertexInputInfo(slang::ShaderReflection* reflection) {
		VertexInputInfo info;

		// Find vertex shader entry point
		for (unsigned i = 0; i < reflection->getEntryPointCount(); i++) {
			auto entry = reflection->getEntryPointByIndex(i);
			if (entry->getStage() == SLANG_STAGE_VERTEX) {
				// Get first parameter (VertexInput struct)
				if (entry->getParameterCount() > 0) {
					auto param = entry->getParameterByIndex(0);
					auto paramType = param->getType();

					if (paramType->getKind() == slang::TypeReflection::Kind::Struct) {
						info.fieldCount = paramType->getFieldCount();

						for (unsigned j = 0; j < paramType->getFieldCount(); j++) {
							auto field = paramType->getFieldByIndex(j);
							std::string name = field->getName() ? field->getName() : ""; 
							if (name == "position") info.hasPosition = true;
							else if (name == "normal") info.hasNormal = true;
							else if (name == "texCoords" ) info.hasTexCoords = true;
							//Including barycentric here for now to account for wireframe shader
							//TOOD see if we can get barycentric coordinates into wireframe shader some other way
							else if (name == "color" || name == "barycentric") info.hasColor = true;
						}
					}
				}
				break;
			}
		}

		return info;
	}

	FragmentOutputInfo getFragmentOutputInfo(slang::ShaderReflection* reflection) {
		FragmentOutputInfo info;
		info.scalarType = slang::TypeReflection::ScalarType::None;
		info.componentCount = 0;

		// Find fragment/pixel shader entry point
		for (unsigned i = 0; i < reflection->getEntryPointCount(); i++) {
			auto entry = reflection->getEntryPointByIndex(i);
			if (entry->getStage() == SLANG_STAGE_FRAGMENT || entry->getStage() == SLANG_STAGE_PIXEL) {
				auto resultVar = entry->getResultVarLayout();
				if (resultVar) {
					auto type = resultVar->getTypeLayout()->getType();

					if (type->getKind() == slang::TypeReflection::Kind::Vector) {
						info.scalarType = type->getScalarType();
						info.componentCount = type->getElementCount();
					}
					else if (type->getKind() == slang::TypeReflection::Kind::Scalar) {
						info.scalarType = type->getScalarType();
						info.componentCount = 1;
					}
				}
				break;
			}
		}

		return info;
	}

	PipelineType identifyPipelineType(slang::ShaderReflection* reflection) {
		auto vertexInput = getVertexInputInfo(reflection);
		auto fragmentOutput = getFragmentOutputInfo(reflection);

		bool isFloat4Output = (fragmentOutput.scalarType == slang::TypeReflection::ScalarType::Float32 &&
			fragmentOutput.componentCount == 4);
		bool isUint32Output = (fragmentOutput.scalarType == slang::TypeReflection::ScalarType::UInt32 &&
			fragmentOutput.componentCount == 1);

		// Vertex: Full input (Position, Normal, TexCoords, Color or barycentric), float4 output
		if (vertexInput.fieldCount == 4 &&
			vertexInput.hasPosition &&
			vertexInput.hasNormal &&
			vertexInput.hasTexCoords &&
			vertexInput.hasColor &&
			isFloat4Output) {
			return PipelineType::Vertex;
		}

		// PhysicsDebug: Only position input, float4 output
		if (vertexInput.fieldCount == 1 &&
			vertexInput.hasPosition &&
			isFloat4Output) {
			return PipelineType::PhysicsDebug;
		}

		// LineVertex: Position + Color input, float4 output
		if (vertexInput.fieldCount == 2 &&
			vertexInput.hasPosition &&
			vertexInput.hasColor &&
			isFloat4Output) {
			return PipelineType::LineVertex;
		}

		// EntityId: Full input (Position, Normal, TexCoords, Color), uint32_t output
		if (vertexInput.fieldCount == 4 &&
			vertexInput.hasPosition &&
			vertexInput.hasNormal &&
			vertexInput.hasTexCoords &&
			vertexInput.hasColor &&
			isUint32Output) {
			return PipelineType::EntityId;
		}

		

		return PipelineType::NONE;
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
};