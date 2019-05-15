#ifndef SheetSprite_h
#define SheetSprite_h

#include <stdio.h>
#include "ShaderProgram.h"

class SheetSprite {
public:
	SheetSprite();
	SheetSprite(GLuint texID, int spriteCountX, int spriteCountY);

	void Draw(ShaderProgram& program, int index, glm::vec3 size);

	GLuint textureID;
	int spriteCountX;
	int spriteCountY;
	float spriteHeight;
	float spriteWidth;
};

#endif

