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

		TransformPropagationPhase = ecs.entity("TransformPropagationPhase")
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
		
		//This updates the world matrix for every 'root' entity, based on the roots transform.
		//A root entity is any entity that has a flecs::Parent relationship to other ents.(different than child_of).
		// The child ents rely their parents WorldMatrix to be updated so they update their own accordingly. 
		ecs.system<const Transform, WorldMatrix>("RootTransformSys")
			.without<flecs::Parent>()
			.kind(TransformPropagationPhase)
			.each([&](flecs::entity ent, const Transform& t, WorldMatrix& worldMat) {
			worldMat.matrix = createWorldMatrix(t);
		});

		
		//This system processes every child entity(ents with flecs::Parent) think wheel in a car model where base body is the parent.
		// It updates their world matrix based on the position of their parent to ensure they are in the correct place.
		ecs.system<MeshComponent, Transform, WorldMatrix, const flecs::Parent>("TransformPropagationSys")
			.kind(TransformPropagationPhase)
			.group_by(flecs::ParentDepth)
			.query_flags(EcsQueryGroupByOrdered)
			.each([&](flecs::entity ent, const MeshComponent& meshComp, const Transform& transform,
				WorldMatrix& worldMat, const flecs::Parent parent) {

			flecs::entity parentEnt = ecs.entity(parent.value);

			//Parent MUST have world matrix so we don't check
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