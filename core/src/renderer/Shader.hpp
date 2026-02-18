#pragma once



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

//TODO REMOVE ONCE ShaderCompiler is complete!!!

namespace shader {

	const char* spirvVersion = "spirv_1_3";

	int generateSpirvComputeShaders(const char* shaderPath, const char* outputPathCS) {

		// STEP 1: Create the Global Session
		Slang::ComPtr<slang::IGlobalSession> slangGlobalSession;
		RETURN_ON_FAIL(slang::createGlobalSession(slangGlobalSession.writeRef()));



		printf("Created slangGlobalSession \n");

		// STEP 2: Configure the Compilation Session
		slang::SessionDesc sessionDesc = {};

		// TargetDesc specifies the output format and version
		slang::TargetDesc targetDesc = {};
		targetDesc.format = SLANG_SPIRV;              // Output SPIRV bytecode (for Vulkan)
		targetDesc.profile = slangGlobalSession->findProfile(spirvVersion);
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
		slang::IModule* slangModule =
			session->loadModule(shaderPath, diagnosticsBlob.writeRef());
		diagnoseIfNeeded(diagnosticsBlob);
		if (!slangModule)
			return SLANG_FAIL;

		printf("Created slang Module \n");

		// STEP 4: Find Entry Points
		// An entry point is a specific function that serves as the "main" for a shader stage
		Slang::ComPtr<slang::IEntryPoint> computeEntryPoint;
		RETURN_ON_FAIL(
			slangModule->findEntryPointByName("computeMain", computeEntryPoint.writeRef()));
		//slangModule->findAndCheckEntryPoint("computeMain", SLANG_STAGE_VERTEX, computeEntryPoint.writeRef() diagnosticsBlob);

		// STEP 5: Compose the Program
		// Combine the module and entry point into a composite program
		// This creates the final executable shader program
		Slang::ComPtr<slang::IComponentType> composedProgram;
		slang::IComponentType* components[] = { slangModule, computeEntryPoint };
		RETURN_ON_FAIL(session->createCompositeComponentType(
			components,
			2,  // number of components
			composedProgram.writeRef()
		));


		// STEP 6: Generate SPIRV Bytecode
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
			printf("Code generation diagnostics:\n%s\n",
				(const char*)diagnosticsComputeShader->getBufferPointer());
		}

		if (SLANG_FAILED(composedProgramResult)) {
			printf("Failed to generate SPIRV code\n");
			return -1;
		}

		if (!spirvComputeShaderCode || spirvComputeShaderCode->getBufferSize() == 0) {
			printf("No SPIRV compute shader code generated\n");
			return -1;
		}

		// STEP 7: Output the SPIRV Bytecode
		printf("Successfully generated SPIRV code: %zu bytes\n", spirvComputeShaderCode->getBufferSize());
		printf("Successfully generated SPIRV compute shader code: %zu bytes\n", spirvComputeShaderCode->getBufferSize());

		// Ensure the directory exists
		std::filesystem::path fullPath(outputPathCS);
		std::filesystem::path directory = fullPath.parent_path();

		if (!std::filesystem::exists(directory)) {

			printf("\033[31mDirectory %s does not exist creating directory\033[0m\n", directory.c_str());

			std::filesystem::create_directories(directory);
		}


		std::ofstream outFileVS(outputPathCS, std::ios::out | std::ios::binary);


		// Write SPIRV binary data to files
		outFileVS.write(
			static_cast<const char*>(spirvComputeShaderCode->getBufferPointer()),
			spirvComputeShaderCode->getBufferSize()
		);

		printf("Successfully created spriv files!\n");

		return 0;
	}

}