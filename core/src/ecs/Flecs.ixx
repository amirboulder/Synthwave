//This file is AI generated.

module;

// INTERNAL-LINKAGE ENTITIES DO NOT CROSS A MODULE BOUNDARY EITHER.
// "import Flecs;" on its own is only valid in a module that makes NO templated
// flecs call. Adding one to an import-only module fails at LINK time with:
//
//     LNK2001 unresolved external symbol
//     "struct flecs::_::placement_new_tag_t const flecs::_::placement_new_tag"
//
// Cause: flecs.h declares
//
//     constexpr placement_new_tag_t placement_new_tag{};        // flecs.h:20172
//
// constexpr implies const, which at namespace scope implies INTERNAL LINKAGE.
// No external symbol for it exists anywhere - not here, not in flecs.lib. Every
// TU that includes flecs.h gets its own private copy. A module that only imports
// has no copy of its own, so nothing can ever resolve the reference. Re-exporting
// it from this module is not an option either: exporting an internal-linkage name
// is ill-formed.
//
// What pulls the reference in: every templated flecs call routes through
// _::type<T>::id(world), which takes &register_lifecycle_actions<T> (flecs.h:31941)
// to install the ctor/copy/move hooks. Those hooks expand FLECS_PLACEMENT_NEW
// (defined flecs.h:20143, used at 23852-23934 and 24407), which names
// placement_new_tag. That covers get, get_mut, try_get, set, add, has, remove,
// emplace, component, system, observer and query - i.e. effectively all real use.
// Naming flecs types as data members does NOT trigger it.
//
// Fix: give that module its own copy via a plain include in its own global module
// fragment. The import then adds nothing and can be dropped:
//
//     #include <flecs.h>   // internal-linkage constexpr + macros
//     export module Foo;
//
// Import-only is therefore restricted to modules that merely declare components:
//
// Every other consumer keeps the textual include. Unlike the Jolt and GLM
// wrappers, this one cannot remove the include from its consumers: the flecs C++
// API is header-only templates instantiated in the caller, not calls into a
// prebuilt .lib. This module buys type visibility, not fewer parses.
//
// Line numbers above are for flecs v4.1.6 (pinned in CMakeLists.txt:329).

//=============================================================================
// Global module fragment.
//
// flecs is built as a separate library (flecs.lib / flecs.dll), so flecs.h MUST
// be included here, above "export module", not in the module purview. That keeps
// every flecs entity attached to the global module and linking against the
// prebuilt library. Including it below "export module Flecs;" would attach those
// entities to this module and produce unresolved externals.
//
// Nothing here is visible to importers by itself - everything the project uses
// is re-exported by name below. If you start using a new flecs type, add a
// using-declaration to the matching block.
//
// MACROS DO NOT CROSS A MODULE BOUNDARY.
// "import Flecs;" gives you types, not macros. The project currently uses two
// flecs macros, and both files must keep a plain include alongside the import:
//
//     #include <flecs.h>   // macros only; types still come from the module
//     import Flecs;
//
//   TransformPropagation.hpp:64   EcsQueryGroupByOrdered   (#define, a bitflag)
//   Editor/src/SceneTree.hpp:68   ecs_each_pair(...)       (function-like macro)
//
// IMPORTANT: any textual #include <flecs.h> must come BEFORE the import in a
// translation unit. Header-then-import is fine; import-then-header produces a
// cascade of redefinition errors ending in "C1116: unrecoverable error importing
// module". pch.h is force-included via /FI, so it is always first.
//=============================================================================

#include <flecs.h>

export module Flecs;


//=============================================================================
// C API (global namespace).
//
// Only what the project actually reaches for. ecs_world_info_t is the one that
// matters for modules - InputManager2.ixx uses it via ecs.get_info(), and a
// module cannot fall back on the PCH. The other three are used only by
// SceneTree.hpp, which needs #include <flecs.h> for ecs_each_pair anyway, but
// exporting them costs nothing and keeps that file's options open.
//=============================================================================

export {
	using ::ecs_world_info_t;
	using ::ecs_iter_t;
	using ::ecs_iter_is_true;

	// Iterating children without the ecs_each_pair MACRO.
	// ecs_each_pair(w,r,t) expands to ecs_each_id(w, ecs_pair(r,t)), and
	// ecs_pair / ECS_PAIR are macros too - none of which cross a module
	// boundary. These three are real FLECS_API functions and do, so a module
	// can iterate children without including <flecs.h>. This is also what
	// flecs' own entity_view::children() calls internally.
	using ::ecs_each_id;
	using ::ecs_children_w_rel;
	using ::ecs_children_next;
	using ::ecs_iter_next;
	using ::ecs_iter_fini;

	//-------------------------------------------------------------------------
	// Builtin entity ids.
	//
	// USE THESE, NOT the flecs::X spellings, inside any .ixx module.
	// These are "FLECS_API extern const ecs_entity_t" - EXTERNAL linkage, so
	// they survive the module boundary with their real values (verified: an
	// importing TU reads EcsChildOf == 294, same as the header).
	//
	// Their C++ wrappers (flecs::ChildOf, flecs::Singleton, ...) are declared
	// "static const flecs::entity_t X = EcsX;" -> INTERNAL linkage, and are
	// deliberately NOT exported. See the warning block further down.
	//-------------------------------------------------------------------------

	// Relationships / builtin tags
	using ::EcsChildOf;
	using ::EcsDependsOn;
	using ::EcsSingleton;
	using ::EcsWildcard;
	using ::EcsDisabled;
	using ::EcsCanToggle;
	using ::EcsParentDepth;
	using ::EcsSystem;
	using ::EcsPhase;
	using ::EcsPrefab;
	using ::EcsIsA;
	using ::EcsName;

	// Pipeline phases
	using ::EcsOnLoad;
	using ::EcsPostLoad;
	using ::EcsPreUpdate;
	using ::EcsOnUpdate;
	using ::EcsOnValidate;
	using ::EcsPostUpdate;
	using ::EcsPreStore;
	using ::EcsOnStore;
	using ::EcsPreFrame;
	using ::EcsPostFrame;

	// Lifecycle events
	using ::EcsOnAdd;
	using ::EcsOnRemove;
	using ::EcsOnSet;
}


//=============================================================================
// C++ API.
//
// Not listed and not needed:
//  - member templates (world::system<...>, world::component<T>, entity::set<T>,
//    iter::field<T>, ...) come with their enclosing class.
//  - builder types returned by those members (system_builder, query_builder)
//    are consumed via chaining or auto, so they are exported as types only.
//=============================================================================

export namespace flecs {

	//--- Core types ----------------------------------------------------------
	using flecs::world;
	using flecs::world_t;
	using flecs::entity;
	using flecs::entity_view;
	using flecs::entity_t;
	using flecs::id;
	using flecs::id_t;
	using flecs::type;
	using flecs::type_t;
	using flecs::table;
	using flecs::table_t;
	using flecs::iter;
	using flecs::field;
	using flecs::ref;

	//--- Queries / systems / observers ---------------------------------------
	using flecs::query;
	using flecs::query_base;
	using flecs::system;
	using flecs::system_builder;
	using flecs::observer;
	using flecs::pipeline;
	using flecs::timer;
	using flecs::term;

	//--- Components / reflection ---------------------------------------------
	using flecs::component;
	using flecs::untyped_component;
	using flecs::serializer;
	using flecs::string;
	using flecs::String;

	//--- EXPERIMENT: entity constants, now that flecs is patched to inline ------
	using flecs::ChildOf;
	using flecs::DependsOn;
	using flecs::Singleton;
	using flecs::Wildcard;
	using flecs::Disabled;
	using flecs::CanToggle;
	using flecs::ParentDepth;
	using flecs::System;
	using flecs::Phase;
	using flecs::Module;
	using flecs::Prefab;
	using flecs::OnLoad;
	using flecs::PostLoad;
	using flecs::PreUpdate;
	using flecs::OnUpdate;
	using flecs::OnValidate;
	using flecs::PostUpdate;
	using flecs::PreStore;
	using flecs::OnStore;
	using flecs::PreFrame;
	using flecs::PostFrame;
	using flecs::OnSet;

	using flecs::string_view;

	//--- Type aliases used as component types --------------------------------
	// Parent is a TYPE alias (using Parent = EcsParent), not an entity constant.
	using flecs::Parent;

	//--- Value macros re-expressed as constants ------------------------------
	// Some flecs macros are just literals, and those CAN cross a module boundary
	// once restated here. EcsQueryGroupByOrdered is (1u << 9u), so
	// TransformPropagation.hpp:64 can use flecs::queryGroupByOrdered instead of
	// the macro and drop its #include <flecs.h>.
	//
	// "inline" is the conforming spelling, not a hard requirement on MSVC.
	// A namespace-scope "constexpr" has internal linkage, and [module.interface]
	// says an exported declaration shall not declare a name with internal
	// linkage. MSVC does not diagnose it here and plain "constexpr" gives the
	// right value too (both measured at 512), but clang is stricter, so prefer
	// inline. Note this is NOT the C5304 case: that warning fires on
	// using-declarations naming a global-module entity, not on a definition
	// written directly in the module purview like this one.
	//
	// The name cannot be EcsQueryGroupByOrdered: flecs.h is included above, so
	// that spelling is still an active macro and would be substituted here.
	//
	// ecs_flags32_t is what query_flags() takes, and using it avoids needing
	// <cstdint> in the global module fragment for std::uint32_t.
	//
	// This trick only works for VALUE macros. It cannot help with function-like
	// macros such as ecs_each_pair(w, r, t).
	inline constexpr ecs_flags32_t queryGroupByOrdered = EcsQueryGroupByOrdered;
	inline constexpr ecs_flags32_t queryGroupByDesc = EcsQueryGroupByDesc;

	//=========================================================================
	// DO NOT EXPORT flecs::Singleton / ChildOf / Phase / OnUpdate / ... HERE.
	//
	// They are declared as
	//     static const flecs::entity_t Singleton = EcsSingleton;   (c_types.hpp)
	// Namespace-scope "static const" gives INTERNAL linkage, and because
	// EcsSingleton is an extern const defined inside flecs.lib, these are
	// DYNAMICALLY initialized - one copy per translation unit.
	//
	// Exporting them compiles (with warning C5304) and LINKS, and then reads
	// ZERO at runtime in the importing TU, because that TU's copy is never
	// initialized. Measured:
	//
	//     Singleton  module=0  header=279      ChildOf   module=0  header=294
	//     OnUpdate   module=0  header=330      Phase     module=0  header=336
	//     Disabled   module=0  header=266      Wildcard  module=0  header=270
	//
	// Entity id 0 means "no entity" in flecs, so .add(flecs::Singleton) would
	// silently do nothing rather than fail. That is why these are omitted and
	// the extern-const Ecs* ids are exported instead (see the C API block above).
	//
	// In a .ixx module write EcsSingleton, not flecs::Singleton.
	//
	// AND THIS IS NOT ONLY ABOUT NAMES YOU WRITE YOURSELF.
	// flecs' own inline functions reference these constants internally, and you
	// cannot rewrite those. Two measured examples:
	//
	//   entity.children(func)  -> calls children(flecs::ChildOf, ...) internally,
	//                             so it silently iterates NOTHING in a TU that
	//                             only imports this module.
	//   flecs::world w;        -> world::init_builtin_components() calls
	//                             meta::_::init(), which does
	//                             world.entity("::flecs::cpp").add(flecs::Module).
	//                             With flecs::Module == 0 that ABORTS at startup:
	//                             "invalid component '#0' passed to add() for
	//                              entity 'flecs.cpp'".
	//
	// THEREFORE: pch.h MUST keep "#include <flecs.h>".
	// Commenting it out makes every non-module TU - including Synthwave.cpp,
	// which constructs the world - source flecs purely from this module, and the
	// program aborts before main() gets going. That exact regression happened;
	// do not "clean up" that include.
	//
	// This module is safe for passing handles around (flecs::world&,
	// flecs::entity, queries, systems). It is NOT a substitute for the header in
	// a TU that constructs a world or relies on builtin ids.
	//
	// flecs::String is likewise not exported - it is only used by
	// RegisterReflectionData.hpp, a non-module header that gets it from pch.h.
	//=========================================================================
}

