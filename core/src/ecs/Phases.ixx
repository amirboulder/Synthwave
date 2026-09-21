module;

#include <flecs.h>

export module Phases;


export struct InputPhase {};
export struct PhysicsPhase {};
export struct AIPhase {};
export struct PlayerPhase {};
export struct TransformPropagationPhase {};
export struct RenderPhase {};

struct InputPhaseDependency {};
struct PhysicsPhaseDependency {};
struct AIPhaseDependency {};
struct PlayerPhaseDependency {};
struct TransformPropagationPhaseDependency {};
struct RenderPhaseDependency {};

/// <summary>
/// Registers All phases
/// Should run before every subsystem constructor and before Registrar
/// </summary>
export struct Phases {

	Phases(flecs::world& ecs) {

		// Each phase has its own dependency, it ensures that
		// 1.phases can be disabled without affecting other phases (disabling is transitive in flecs)
		// 2.Phases can run in the order we want regardless of creation order (no longer relevant but good to know) 
		flecs::entity inputPhaseDependency = ecs.entity<InputPhaseDependency>();

		flecs::entity physicsPhaseDependency = ecs.entity<PhysicsPhaseDependency>()
			.depends_on(inputPhaseDependency);

		flecs::entity aiPhaseDependency = ecs.entity<AIPhaseDependency>()
			.depends_on(physicsPhaseDependency);

		flecs::entity playerPhaseDependency = ecs.entity<PlayerPhaseDependency>()
			.depends_on(aiPhaseDependency);

		flecs::entity transformPropPhaseDependency = ecs.entity<TransformPropagationPhaseDependency>()
			.depends_on(playerPhaseDependency);

		flecs::entity renderPhaseDependency = ecs.entity<RenderPhaseDependency>()
			.depends_on(transformPropPhaseDependency);

		ecs.entity(flecs::PostFrame).depends_on(renderPhaseDependency);



		flecs::entity inputPhase = ecs.entity<InputPhase>()
			.add(flecs::Phase)
			.depends_on(inputPhaseDependency);

		flecs::entity physicsPhase = ecs.entity<PhysicsPhase>()
			.add(flecs::Phase)
			.depends_on(physicsPhaseDependency);

		flecs::entity aiPhase = ecs.entity<AIPhase>()
			.add(flecs::Phase)
			.depends_on(aiPhaseDependency);

		flecs::entity playerPhase = ecs.entity<PlayerPhase>()
			.add(flecs::Phase)
			.depends_on(playerPhaseDependency);

		flecs::entity transformPropagationPhase = ecs.entity<TransformPropagationPhase>()
			.add(flecs::Phase)
			.depends_on(transformPropPhaseDependency);

		flecs::entity renderPhase = ecs.entity<RenderPhase>()
			.add(flecs::Phase)
			.depends_on(renderPhaseDependency);


		//disable gameplay phases before game starts
		disableGameplayPhases(ecs);

		
		// Disable most the default phases so we don't see them when printing systems

		//ecs.entity(flecs::PreFrame).disable();
		ecs.entity(flecs::OnLoad).disable();
		ecs.entity(flecs::PostLoad).disable();
		ecs.entity(flecs::PreUpdate).disable();
		ecs.entity(flecs::OnUpdate).disable();
		ecs.entity(flecs::OnValidate).disable();
		ecs.entity(flecs::PostUpdate).disable();
		ecs.entity(flecs::PreStore).disable();
		ecs.entity(flecs::OnStore).disable();
		//ecs.entity(flecs::PostFrame).disable();
	}


	//TODO maybe we should check if the phases exist before letting people disable them ???
	static void disableGameplayPhases(flecs::world& ecs) {

		ecs.entity<PhysicsPhase>().disable();
		ecs.entity<AIPhase>().disable();
		ecs.entity<PlayerPhase>().disable();
	}

	static void enableGameplayPhases(flecs::world& ecs) {

		ecs.entity<PhysicsPhase>().enable();
		ecs.entity<AIPhase>().enable();
		ecs.entity<PlayerPhase>().enable();
	}

	
};
