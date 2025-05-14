#pragma once

#include "renderer/Model.hpp"
#include "physics/physics.hpp"
#include "core/src/player.hpp"

class Entity {

public:

	//TODO make const
	uint32_t id;

	BodyID physicsID;

	uint32_t modelIndex;

	friend class EntityFactory;

private:

	Entity()
	{

	}


	// factory constructor
	Entity(uint32_t id, BodyID physicsID, uint32_t modelIndex)
		: id(id), physicsID(physicsID), modelIndex(modelIndex)

	{
		//cout << "entity created in factory with id : " << id << '\n';
		//cout << "physicsID index : " << physicsID.GetIndex() << '\n';
		//cout << "Model id : " << modelIndex << '\n';
	}

	Entity(const Entity& other)
		: id(other.id), physicsID(other.physicsID), modelIndex(other.modelIndex)
	{
		printf("\033[33mEntity copy constructor called!\033[0m\n");
	}

	Entity(Entity&& other) noexcept
		: id(other.id), physicsID(std::move(other.physicsID)), modelIndex(other.modelIndex)
	{
		printf("\033[33mEntity Move constructor called!\033[0m\n");

		// Optionally reset the source object (not always needed)
		//other.id = 0;
		//other.modelIndex = 0;
	}

};