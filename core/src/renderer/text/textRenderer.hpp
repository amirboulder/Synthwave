#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdio.h>


class TextRenderer {

public:

	TextRenderer() {

        if (!TTF_Init()) {
            SDL_Log("Couldn't initialise SDL_ttf: %s\n", SDL_GetError());
        }
	}

 

};
