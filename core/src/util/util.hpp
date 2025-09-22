#pragma once

#include <SDL3/SDL.h>

void check_error_bool(const bool res)
{
    if (!res) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", SDL_GetError());
    }
}

template<typename T>
T* check_error_ptr(T* ptr) {
    if (!ptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", SDL_GetError());
    }
    return ptr;
}