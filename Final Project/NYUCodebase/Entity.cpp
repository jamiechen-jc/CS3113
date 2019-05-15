#include "Entity.h"

Entity::Entity() {
	position = glm::vec3(0.0f, 0.0f, 0.0f);
	velocity = glm::vec3(0.0f, 0.0f, 0.0f);
	acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
	size = glm::vec3(0.5f, 0.5f, 0.5f);
	entityType = NONE;
	isStatic = true;
	resetCollisionFlags();
};

Entity::Entity(EntityType entityType, bool isStatic, SheetSprite sprite) : entityType(entityType), isStatic(isStatic), sprite(sprite) {
	position = glm::vec3(0.0f, 0.0f, 0.0f);
	velocity = glm::vec3(0.0f, 0.0f, 0.0f);
	acceleration = glm::vec3(0.0f, -0.981f, 0.0f);
	size = glm::vec3(1.0f, 1.0f, 1.0f);
	resetCollisionFlags();
};

void Entity::Draw(ShaderProgram& program, int index) {
	glm::mat4 modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, position);
	modelMatrix = glm::scale(modelMatrix, size);
	if (velocity.x < 0)
		modelMatrix = glm::scale(modelMatrix, glm::vec3(-1.0f, 1.0f, 1.0f));
	program.SetModelMatrix(modelMatrix);
	sprite.Draw(program, index);
}

void Entity::Render(ShaderProgram& program) {}

void Entity::Update(float elasped) {
}

bool Entity::collidesWith(Entity& entity) {
	float penetration_x = fabs(position.x - entity.position.x) - ((size.x + entity.size.x) / 2);
	float penetration_y = fabs(position.y - entity.position.y) - ((size.y + entity.size.y) / 2);

	return (penetration_x < 0 && penetration_y < 0);
}

bool Entity::collisionX(Entity& entity, float pos) {
	if (collidesWith(entity)) {
		if (velocity.x < 0.0f) {
			float penetration = fabs((position.x - size.x / 2) - pos);
			position.x += penetration;
			collidedLeft = true;
		}
		else if (velocity.x > 0) {
			float penetration = fabs((position.x + size.x / 2) - pos);
			position.x -= penetration;
			collidedRight = true;
		}
		velocity.x = 0;
		return true;
	}
	return false;
}

bool Entity::collisionY(Entity& entity, float pos) {
	if (collidesWith(entity)) {
		if (velocity.y < 0) {
			float penetration = fabs(pos - (position.y - size.y / 2));
			position.y += penetration;
			collidedBottom = true;
		}
		else if (velocity.y > 0) {
			float penetration = fabs(pos - (position.y + size.y / 2));
			position.y -= penetration;
			collidedTop = true;
		}
		velocity.y = 0;
		return true;
	}
	return false;
}

void Entity::resetCollisionFlags() {
	collidedBottom = false;
	collidedTop = false;
	collidedLeft = false;
	collidedRight = false;
}

std::vector<std::pair<float, float>> Entity::getPoints() {
	float x = position.x;
	float y = position.y;
	float halfWidth = size.x / 2;
	float halfHeight = size.y / 2;

	std::vector<std::pair<float, float>> cornerPoints;

	std::pair<float, float> point1(x - halfWidth, y + halfHeight);
	std::pair<float, float> point2(x + halfWidth, y + halfHeight);
	std::pair<float, float> point3(x - halfWidth, y - halfHeight);
	std::pair<float, float> point4(x + halfWidth, y - halfHeight);

	cornerPoints.push_back(point1);
	cornerPoints.push_back(point2);
	cornerPoints.push_back(point3);
	cornerPoints.push_back(point4);

	return cornerPoints;
}