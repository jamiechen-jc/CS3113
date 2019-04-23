#ifndef Entity_h
#define Entity_h

#include <stdio.h>
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "SheetSprite.h"

enum EntityType { ENTITY_PLAYER, ENTITY_ENEMY, ENTITY_COIN, NONE };

class Entity {
public:
	Entity();
	Entity(EntityType entityType, bool isStatic, SheetSprite sprite);

	void Update(float elapsed);
	//void Render(ShaderProgram& program);
	void Draw(ShaderProgram& program, int index);
	bool collidesWith(Entity& entity);
	bool collisionX(Entity& entity, float pos);
	bool collisionY(Entity& entity, float pos);
	void resetCollisionFlags();
	
	SheetSprite sprite;

	glm::vec3 position;
	glm::vec3 velocity;
	glm::vec3 acceleration;
	glm::vec3 size;

	bool isCaught;
	bool isStatic;
	EntityType entityType;

	bool collidedTop;
	bool collidedBottom;
	bool collidedLeft;
	bool collidedRight;
};

#endif