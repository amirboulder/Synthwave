#pragma once

namespace CMN {

	void flushMouseMovement() {
		// flushing all the mouse movement accumulated during pause/load to avoid camera jerk
		float dx, dy;
		SDL_GetRelativeMouseState(&dx, &dy);
	}


}



