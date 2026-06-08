#pragma once 

class TransformPropagation {

public:

	flecs::world& ecs;

	flecs::entity TransformPropagationPhase;

	TransformPropagation(flecs::world& ecs)
		:ecs(ecs)
	{	

		registerPhase();
		registerSystems();

		LogSuccess(LOG_APP, "TransformPropagation Initialized");
	}

	bool registerPhase() {

		// Each phase has its own dependency, it ensures that
		// 1.phases can be disabled without affecting other phases (disabling is transitive in flecs)
		// 2.Phases can run in the order we want regardless of creation order 
		//PhaseDependencies depend on each other, that's handled in StateManager.RegisterPhaseDependencies()
		// that way phases created earlier in initialization can depend on phases created after them
		flecs::entity transformPropagationPhaseDependency = ecs.entity("TransformPropagationPhaseDependency");

		TransformPropagationPhase = ecs.entity("RenderPhase")
			.add(flecs::Phase)
			.depends_on(transformPropagationPhaseDependency);

		if (!transformPropagationPhaseDependency || !TransformPropagationPhase) {
			LogError(LOG_APP, "Creating TransformPropagationPhase Failed");
		}

		return true;
	}

	void registerSystems() {

		TransformPropagationSystem();
	}

	void TransformPropagationSystem() {
		
		ecs.system<const Transform, WorldMatrix>("RootTransformSys")
			.without<flecs::Parent>()
			.kind(TransformPropagationPhase)
			.each([&](const Transform& t, WorldMatrix& worldMat) {
			worldMat.matrix = createWorldMatrix(t);
		});


		ecs.system<MeshComponent, Transform, WorldMatrix, const flecs::Parent>("TransformPropagationSys")
			.kind(TransformPropagationPhase)
			.group_by(flecs::ParentDepth)
			.each([&](const MeshComponent& meshComp, const Transform& transform,
				WorldMatrix& worldMat, const flecs::Parent parent) {

			flecs::entity parentEnt = ecs.entity(parent.value);

			//Assuming parent has worldMatrix
			const glm::mat4& parentWorldMat = parentEnt.get<WorldMatrix>().matrix;

			worldMat.matrix = parentWorldMat * createWorldMatrix(transform);
		});
	}


	glm::mat4 createWorldMatrix(const Transform& transform) {

		glm::mat4 modelTranslation = glm::translate(glm::mat4(1.0f), transform.position);
		glm::mat4 modelRotation = glm::toMat4(transform.rotation);
		glm::mat4 modelScale = glm::scale(glm::mat4(1.0f), transform.scale);
		return modelTranslation * modelRotation * modelScale;
	}
	
};