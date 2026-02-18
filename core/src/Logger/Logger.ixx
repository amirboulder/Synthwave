module;

#include <SDL3/SDL.h>
#include <cstdarg>
#include <iostream>

/// <summary>
/// A wrapper around SDL_Log
/// allows for color coded log messages based on log category
/// </summary>
export module Logger;

//Colors for logging
#define SUCCESS "\x1b[32m"
#define WARN "\x1b[33m"
#define ERR     "\x1b[31m"
#define RESET "\x1b[0m"
#define SYNTH "\x1b[35m"

export typedef enum {
	LOG_APP = SDL_LOG_CATEGORY_APPLICATION,
	LOG_ERR = SDL_LOG_CATEGORY_ERROR,
	LOG_SYS = SDL_LOG_CATEGORY_SYSTEM,
	LOG_AUD = SDL_LOG_CATEGORY_AUDIO,
	LOG_VID = SDL_LOG_CATEGORY_VIDEO,
	LOG_RENDER = SDL_LOG_CATEGORY_RENDER,
	LOG_INP = SDL_LOG_CATEGORY_INPUT,
	LOG_TST = SDL_LOG_CATEGORY_TEST,
	LOG_GPU = SDL_LOG_CATEGORY_GPU,
} LogCategory;

// Helper to format variadic args into a std::string
static std::string FormatString(const char* fmt, va_list args) {
	va_list copy;
	va_copy(copy, args);
	int size = vsnprintf(nullptr, 0, fmt, copy); // get required buffer size
	va_end(copy);

	std::string result(size + 1, '\0');
	vsnprintf(result.data(), size + 1, fmt, args);
	result.resize(size); // strip null terminator
	return result;
}

export void LogError(LogCategory category, SDL_PRINTF_FORMAT_STRING const char* fmt, ...) SDL_PRINTF_VARARG_FUNC(2) {
    va_list args;
    va_start(args, fmt);
    std::string msg = ERR + FormatString(fmt, args) + RESET;
    va_end(args);
    SDL_LogError(category, "%s", msg.c_str());
}

export void LogWarn(LogCategory category, SDL_PRINTF_FORMAT_STRING const char* fmt, ...) SDL_PRINTF_VARARG_FUNC(2) {
    va_list args;
    va_start(args, fmt);
    std::string msg = WARN + FormatString(fmt, args) + RESET;
    va_end(args);
    SDL_LogWarn(category, "%s", msg.c_str());
}

export void LogInfo(LogCategory category, SDL_PRINTF_FORMAT_STRING const char* fmt, ...) SDL_PRINTF_VARARG_FUNC(2) {
    va_list args;
    va_start(args, fmt);
    std::string msg = FormatString(fmt, args);
    va_end(args);
    SDL_LogInfo(category, "%s", msg.c_str());
}

export void LogDebug(LogCategory category, SDL_PRINTF_FORMAT_STRING const char* fmt, ...) SDL_PRINTF_VARARG_FUNC(2) {
    va_list args;
    va_start(args, fmt);
    std::string msg = FormatString(fmt, args);
    va_end(args);
    SDL_LogDebug(category, "%s", msg.c_str());
}

export void LogVerbose(LogCategory category, SDL_PRINTF_FORMAT_STRING const char* fmt, ...) SDL_PRINTF_VARARG_FUNC(2) {
    va_list args;
    va_start(args, fmt);
    std::string msg = FormatString(fmt, args);
    va_end(args);
    SDL_LogVerbose(category, "%s", msg.c_str());
}

export void LogTrace(LogCategory category, SDL_PRINTF_FORMAT_STRING const char* fmt, ...) SDL_PRINTF_VARARG_FUNC(2) {
    va_list args;
    va_start(args, fmt);
    std::string msg = FormatString(fmt, args);
    va_end(args);
    SDL_LogTrace(category, "%s", msg.c_str());
}

export void LogSuccess(LogCategory category, SDL_PRINTF_FORMAT_STRING const char* fmt, ...) SDL_PRINTF_VARARG_FUNC(2) {
    va_list args;
    va_start(args, fmt);
    std::string msg = SUCCESS + FormatString(fmt, args) + RESET;
    va_end(args);
    SDL_LogDebug(category, "%s", msg.c_str());
}

export void LogSynth(LogCategory category, SDL_PRINTF_FORMAT_STRING const char* fmt, ...) SDL_PRINTF_VARARG_FUNC(2) {
    va_list args;
    va_start(args, fmt);
    std::string msg = SYNTH + FormatString(fmt, args) + RESET;
    va_end(args);
    SDL_LogInfo(category, "%s", msg.c_str());
}