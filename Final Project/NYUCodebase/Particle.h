#ifndef Particle_h
#define Particle_h

#include "glm/gtc/matrix_transform.hpp"

class Particle {
public:
	glm::vec3 position;
	glm::vec3 velocity;
	float lifetime;
	float sizeDeviation;
};

#endif