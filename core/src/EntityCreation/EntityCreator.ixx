module;

#include <string>
#include <vector>

#include <flecs.h>

export module EntityCreator;

import GLM;
import Components;
import GraphicsComponents;
import EntityFactory;

export struct EntityCreationCommand {

	std::string entityName;
	flecs::entity entityParent;
	Transform transform;
	glm::vec3 linearVelocity = glm::vec3(0);
	glm::vec3 angularVelocity = glm::vec3(0);
	EntityType entityType = EntityType::Empty;

};

export struct EntityCreationQueue {

	std::vector<EntityCreationCommand> queue;
};



/// <summary>
/// This class allows various systems to create entities by pushing EntityCreationCommands to EntityCreationQueue.
/// Every ECS frame it goes through the queue and creates the entities and clears the queue.
/// This system should be created after inputManager to make sure it runs after it during flecs::PreFrame phase.
/// To Ensure that entity creation commands are processed the same frame they are created.
/// Note: This should always be used instead of entity factory to avoid circular dependencies, which C++ modules do not allow.
/// </summary>
export class EntityCreator {

public:

	flecs::system createEntitiesSys;
	flecs::world& ecs;

	EntityCreator(flecs::world& ecs)
		:ecs(ecs)
	{
		 
		//Register queue as Singleton
		ecs.component<EntityCreationQueue>().add(flecs::Singleton);
		ecs.set<EntityCreationQueue>({});

		removeStaleContactsSystem();
	}


	void removeStaleContactsSystem() {

		createEntitiesSys = ecs.system("CreateEntitiesSys")
			.kind(flecs::PreFrame)
			.immediate()
			.run([&](flecs::iter& it) {


			std::vector<EntityCreationCommand>& queue = ecs.get_mut<EntityCreationQueue>().queue;

			//TODO wire-in the rest of the entity creation code.
			for (EntityCreationCommand& command : queue) {

				switch (command.entityType)
				{
				case EntityType::COUNT:
					break;
				case EntityType::Camera:
					break;
				case EntityType::Light:
					break;
				case EntityType::Cube:
					break;
				case EntityType::Sensor:
					break;
				case EntityType::Cylinder:
					break;
				case EntityType::Sphere:
					EntityFactory::createSphereEntity(ecs, command.entityParent, command.entityName, command.transform, command.linearVelocity, command.angularVelocity);
					break;
				case EntityType::Mountain:
					break;
				case EntityType::StaticMesh:
					break;
				case EntityType::Grid:
					break;
				case EntityType::Capsule:
					break;
				case EntityType::Actor:
					break;
				case EntityType::Snake:
					break;
				case EntityType::RobotArm:
					break;
				case EntityType::JoltRagdollExample:
					break;
				case EntityType::RagdollKinematic:
					break;
				case EntityType::RagdollForce:
					break;
				case EntityType::Ragdoll:
					break;
				case EntityType::Humanoid:
					break;
				case EntityType::Player:
					break;
				case EntityType::BoxCar:
					break;
				case EntityType::Scene:
					break;
				case EntityType::Game:
					break;
				case EntityType::Generic:
					break;
				case EntityType::Empty:
					break;
				default:
					break;
				}
			}

			queue.clear();
	
		});

	}

};



