#ifndef SheetSprite_h
#define SheetSprite_h

#include <stdio.h>
#include "ShaderProgram.h"

class SheetSprite {
public:
	SheetSprite();
	SheetSprite(unsigned int, float, float, float ,float, float);

	//void Draw(ShaderProgram& program);
	void Draw(ShaderProgram& program, int index, int scountx, int scounty);

	float size;
	unsigned int textureID;
	float u;
	float v;
	float width;
	float height;
};

#endif

