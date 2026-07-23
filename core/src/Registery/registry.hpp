#pragma once

using RegisterFn = void(*)(flecs::world&);

inline std::vector<RegisterFn>& GetGameRegistry() {
	static std::vector<RegisterFn> fns;
	return fns;
}

struct AutoRegister {
	AutoRegister(RegisterFn fn) { GetGameRegistry().push_back(fn); }
};

#define REGISTER_GAME_MODULE(fn) \
    static AutoRegister _auto_##fn{fn};

class Registrar {

public:

	flecs::world& ecs;

	Registrar(flecs::world& ecs)
		:ecs(ecs)
	{
		for (auto& fn : GetGameRegistry()) {
			fn(ecs);
		}
	}
};

