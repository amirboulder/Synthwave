module;

export module InputComponents;

import GLM;

export enum class Direction { forward, backward };

export struct UserInput {
	glm::vec2 direction = glm::vec2(0);
	float offsetX = 0.0f;
	float offsetY = 0.0f;
	float magnitude = 0.0f;         // 0-1, for speed scaling
	bool jump = false;
	bool jumpConsumed = true;
};