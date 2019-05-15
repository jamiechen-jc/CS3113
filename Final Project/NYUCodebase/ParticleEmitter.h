#ifndef ParticleEmitter_h
#define ParticleEmitter_h

#include "Particle.h"
#include "ShaderProgram.h"
#include <vector>

class ParticleEmitter {
public:
	ParticleEmitter(unsigned int particleCount);
	ParticleEmitter();
	~ParticleEmitter();


	void Update(float elapsed);
	void Render(ShaderProgram& program);
	glm::vec3 position;
	glm::vec3 gravity;
	float maxLifetime;
	float startSize;
	float endSize;
	float sizeDeviation;

	std::vector<Particle> particles;
};

#endif