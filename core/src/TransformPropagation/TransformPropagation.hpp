#pragma once 

class TransformPropagation {

public:

	flecs::world& ecs;

	TransformPropagation(flecs::world& ecs)
		:ecs(ecs)
	{	

		registerSystems();

		LogSuccess(LOG_APP, "TransformPropagation Initialized");
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
			.kind<TransformPropagationPhase>()
			.each([&](flecs::entity ent, const Transform& t, WorldMatrix& worldMat) {
			worldMat.matrix = createWorldMatrix(t);
		});

		
		//This system processes every child entity(ents with flecs::Parent) think wheel in a car model where base body is the parent.
		// It updates their world matrix based on the position of their parent to ensure they are in the correct place.
		ecs.system<MeshComponent, Transform, WorldMatrix, const flecs::Parent>("TransformPropagationSys")
			.kind<TransformPropagationPhase>()
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