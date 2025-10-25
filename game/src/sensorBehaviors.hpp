#pragma once

#include "core/src/pch.h"

void sensor1Behavoir(flecs::world & ecs,flecs::entity self, flecs::entity other) {

	SDL_Log("Sensor %s touched %s", self.name().c_str(), other.name().c_str());

}
