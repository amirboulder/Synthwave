#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <iostream>

class SDLWindow {

public:

	SDL_Window* window;

	SDLWindow(int width,int height) {

		
        init();
        createWindow(width,height);
		

	}

    int init() {

        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "Failed to initialize SDL: " << SDL_GetError() << '\n';
            return 1;
        }

        // Set OpenGL version to 4.6 Core
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);


    }

	int createWindow(int width, int height) {

        window = SDL_CreateWindow(
            "An SDL3 window",                  // window title
            width,                               // width, in pixels
            height,                               // height, in pixels
            SDL_WINDOW_OPENGL                  // flags - see below
        );

        if (!window) {
            std::cerr << "Failed to create window: " << SDL_GetError() << '\n';
            return 1;
        }

        SDL_GLContext glContext = SDL_GL_CreateContext(window);
        if (!glContext) {
            std::cerr << "Failed to create OpenGL context: " << SDL_GetError() << '\n';
            return 1;
        }

        // Load OpenGL functions with GLAD
        if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
            std::cerr << "Failed to initialize GLAD\n";
            return 1;
        }

        SDL_SetWindowRelativeMouseMode(window,true);

        return 0;
	}


    void destroy() {

        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    
};