#pragma once

//PLace All physics Components here


using contactAddedFunction = std::function<void(
	flecs::world& ecs,
	flecs::entity self,
	flecs::entity other,
	const Body& selfBody, 
	const Body& otherBody, 
	const ContactManifold& inManifold, 
	ContactSettings& ioSettings)> ;


struct ContactAddedBehavior {
	contactAddedFunction onContactAdded;
};