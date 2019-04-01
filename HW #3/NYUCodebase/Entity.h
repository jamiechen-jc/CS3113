#ifndef Entity_h
#define Entity_h

#include <stdio.h>
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "SheetSprite.h"

enum STATE { ALIVE, DEAD };

class Entity {
public:
	Entity();
	Entity(glm::vec3, glm::vec3, glm::vec3, float, SheetSprite);
	void Draw(ShaderProgram& program, int index, int scountx, int scounty);
	void Update(float elapsed);
	
	glm::vec3 position;
	glm::vec3 velocity;
	glm::vec3 size;
	float rotation;
	SheetSprite sprite;
	float health;
	enum STATE state;
};

#endif