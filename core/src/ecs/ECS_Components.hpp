#pragma once

// All Components that depend on flecs should go here.




struct ActorBehavior {

	std::function<void(flecs::world& ecs, flecs::entity self)> actorUpdate;

};

//These two are the same thing get rid of one
//TODO find a better name for this
struct HudRender {
	std::function<void(flecs::world& ecs)> draw;
};
struct Render {
	std::function<void(flecs::world& ecs)> draw;
};


struct PlayerRef { flecs::entity value = flecs::entity::null(); };
struct PlayerCamRef { flecs::entity value = flecs::entity::null(); };


//TODO MOVE THIS
struct HighlightedEntRef {
	flecs::entity ent;
};
