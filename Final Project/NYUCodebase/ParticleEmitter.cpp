#include "ParticleEmitter.h"
#include "Helper.h"
#include <vector>

using namespace std;

ParticleEmitter::ParticleEmitter() {

}

ParticleEmitter::ParticleEmitter(unsigned int particleCount) {

}

ParticleEmitter::~ParticleEmitter() {

}

void ParticleEmitter::Render(ShaderProgram& program) {
	vector<float> vertices;
	vector<float> texCoords;

	for (int i = 0; i < particles.size(); i++) {
		float m = (particles[i].lifetime / maxLifetime);
		float size = lerp(startSize, endSize, m) + particles[i].sizeDeviation;

		vertices.insert(vertices.end(), {
		particles[i].position.x - size, particles[i].position.y + size,
		particles[i].position.x - size, particles[i].position.y - size,
		particles[i].position.x + size, particles[i].position.y + size,

		particles[i].position.x + size, particles[i].position.y + size,
		particles[i].position.x - size, particles[i].position.y - size,
		particles[i].position.x + size, particles[i].position.y - size
			});

		texCoords.insert(texCoords.end(), {
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,

		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f
			});
		/*
		for (int j = 0; j < 6; j++) {
			colors.push_back(lerp(startColor.r, endColor.r, m));
			colors.push_back(lerp(startColor.g, endColor.g, m));
			colors.push_back(lerp(startColor.b, endColor.b, m));
			colors.push_back(lerp(startColor.a, endColor.a, m));
		}
		*/
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices.data());
		glEnableVertexAttribArray(program.positionAttribute);
		//glVertexAttribPointer(colorAttribute, 4, GL_FLOAT, false, 0, colors.data());
		//glEnableVertexAttribArray(colorAttribute);
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords.data());
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 2);
	}
}
