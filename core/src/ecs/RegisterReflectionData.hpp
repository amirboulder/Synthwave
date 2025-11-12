#pragma once

#include "core/src/pch.h"

//TODO move to serialize class 
void registerReflectionData(flecs::world & ecs){

    
    ecs.component<glm::vec3>()
        .member<float>("x")
        .member<float>("y")
        .member<float>("z");

    ecs.component<glm::quat>()
        .member<float>("x")
        .member<float>("y")
        .member<float>("z")
        .member<float>("w");

   ecs.component<Transform>()
        .member<glm::vec3>("position")
        .member<glm::quat>("rotation")
        .member<glm::vec3>("scale");

   // Register reflection for std::string
   ecs.component<std::string>()
       .opaque(flecs::String) // Opaque type that maps to string
       .serialize([](const flecs::serializer* s, const std::string* data) {
       const char* str = data->c_str();
       return s->value(flecs::String, &str); // Forward to serializer
   })
       .assign_string([](std::string* data, const char* value) {
       *data = value; // Assign new value to std::string
   });
   
   ecs.component<ModelSourceRef>()
       .member<std::string>("name");

   ecs.component<ObjectType>()
       .member<std::string>("name");



}