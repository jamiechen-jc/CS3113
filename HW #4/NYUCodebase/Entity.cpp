#include "Entity.h"

Entity::Entity() {
	position = glm::vec3(0.0f, 0.0f, 0.0f);
	velocity = glm::vec3(0.0f, 0.0f, 0.0f);
	acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
	size = glm::vec3(1.0f, 1.0f, 1.0f);
	entityType = NONE;
	isStatic = true;
	isCaught = false;
	resetCollisionFlags();
};

Entity::Entity(EntityType entityType, bool isStatic, SheetSprite sprite) : entityType(entityType), isStatic(isStatic), sprite(sprite) {
	position = glm::vec3(0.0f, 0.0f, 0.0f);
	velocity = glm::vec3(0.0f, 0.0f, 0.0f);
	acceleration = glm::vec3(0.0f, -0.981f, 0.0f);
	size = glm::vec3(0.2f, 0.2f, 0.2f);
	isCaught = false;
	resetCollisionFlags();
};

void Entity::Draw(ShaderProgram& program, int index) {
	glm::mat4 modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, position);
	program.SetModelMatrix(modelMatrix);
	sprite.Draw(program, index);
}

void Entity::Update(float elasped) {
/*	if (!isStatic) {
		switch (entityType) {
			case ENTITY_PLAYER:
				if (!collidedBottom) {

				}
			default:
				break;
		}
	}*/
}

bool Entity::collidesWith(Entity& entity) {
	float penetration_x = fabs(position.x - entity.position.x) - ((size.x + entity.size.x) / 2);
	float penetration_y = fabs(position.y - entity.position.y) - ((size.y + entity.size.y) / 2);

	return (penetration_x < 0 && penetration_y < 0);
}

bool Entity::collisionX(Entity& entity, float pos) {
	if (collidesWith(entity)) {
		if (velocity.x < 0.0f) {
			float penetration = fabs(pos - (position.x - size.x / 2));
			position.x += penetration;
			collidedLeft = true;
		}
		else if (velocity.x > 0) {
			float penetration = fabs(pos - (position.x - size.x / 2));
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
			float penetration = fabs(pos - (position.y - size.y / 2));
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