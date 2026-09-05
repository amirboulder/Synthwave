#pragma once

//TODO put this in util or just in inputManager
namespace CMN {

	
	inline void flushMouseMovement() {
		// flushing all the mouse movement accumulated during pause/load to avoid camera jerk
		float dx, dy;
		SDL_GetRelativeMouseState(&dx, &dy);
	}

}



