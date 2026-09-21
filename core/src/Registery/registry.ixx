module;

#include <vector>
#include <flecs.h>

//This module allows flecs::modules to register themselves by doing the following
// const RegisterModule<PlayerSystems> registerPlayerSystems;
//This allows for gameplay systems to be registered without the engine needing to know their names
//Important Note: c++ modules need to be in the projects import graph in order for the above code to be registerd.
//Perhaps create a game modules file in game folder so that gameplay system can register there and in turn just import that.
export module Registry;

export using RegisterFn = void(*)(flecs::world&);

std::vector<RegisterFn>& gameRegistry() {
    static std::vector<RegisterFn> fns; 
    return fns;
}


export template<class T>
struct RegisterModule {
    RegisterModule() {
        gameRegistry().push_back(+[](flecs::world& ecs) { ecs.import<T>(); });
    }
};

export struct AutoRegister {
    explicit AutoRegister(RegisterFn fn) { gameRegistry().push_back(fn); }
};

export class Registrar {
public:
    flecs::world& ecs;

    explicit Registrar(flecs::world& ecs)
        : ecs(ecs)
    {
        for (RegisterFn fn : gameRegistry()) fn(ecs);
    }
};