#include "SheetSprite.h"
#include "ShaderProgram.h"

SheetSprite::SheetSprite() {}
SheetSprite::SheetSprite(GLuint texID, int spriteCountX, int spriteCountY) : textureID(texID), spriteCountX(spriteCountX), spriteCountY(spriteCountY) {
	spriteHeight = 1.0 / (float) spriteCountX;
	spriteWidth = 1.0f / (float) spriteCountY;
}

void SheetSprite::Draw(ShaderProgram &program, int index, glm::vec3 size) {
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	
	float u = (float)(((int)index) % spriteCountX) / (float)spriteCountX;
	float v = (float)(((int)index) / spriteCountX) / (float)spriteCountY;
	float spriteWidth = 1.0 / (float)spriteCountX;
	float spriteHeight = 1.0 / (float)spriteCountY;

	float texCoords[] = {
		u, v + spriteHeight,
		u + spriteWidth, v,
		u, v,
		u + spriteWidth, v,
		u, v + spriteHeight,
		u + spriteWidth, v + spriteHeight
	};

	float vertices[] = {
		-0.5f * size.x, -0.5f * size.x,
		0.5f * size.x, 0.5f * size.x,
		-0.5f * size.x, 0.5f * size.x,
		0.5f * size.x, 0.5f * size.x,
		-0.5f * size.x, -0.5f * size.x,
		0.5f * size.x, -0.5f * size.x
	};
	
	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program.positionAttribute);

	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program.texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);
}