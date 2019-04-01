#include "Entity.h"

Entity::Entity() {};
Entity::Entity(glm::vec3 position, glm::vec3 velocity, glm::vec3 size, float rotation, SheetSprite sprite) : position(position), velocity(velocity), size(size), rotation(rotation), sprite(sprite), health(1), state(ALIVE) {
};

void Entity::Draw(ShaderProgram& program, int index, int scountx, int scounty) {
	glm::mat4 modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, position);
	program.SetModelMatrix(modelMatrix);
	sprite.Draw(program, index, scountx, scounty);
}	

void Entity::Update(float elapsed) {

}