#pragma once

//Colors for logging
#define GOOD "\x1b[32m"
#define WARN "\x1b[33m"
#define ERROR "\x1b[31m"
#define RESET "\x1b[0m"
#define SYNTH "\x1b[35m"

namespace CMN {

	void flushMouseMovement() {
		// flushing all the mouse movement accumulated during pause/load to avoid camera jerk
		float dx, dy;
		SDL_GetRelativeMouseState(&dx, &dy);
	}


}



