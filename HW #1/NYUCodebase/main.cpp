#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "ShaderProgram.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <string>

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

GLuint LoadTexture(const char *filePath) {
	int w, h, comp;
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);

	if (image == NULL) {
		std::cout << "Unable to load image. Make sure the path is correct\n";
		assert(false);
	}

	GLuint retTexture;
	glGenTextures(1, &retTexture);
	glBindTexture(GL_TEXTURE_2D, retTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	stbi_image_free(image);
	return retTexture;
}

bool canMove(float x, float border, std::string direction) {
	if (direction == "left" || direction == "down")
		return (x > border);
	if (direction == "right" || direction == "up")
		return (x < border);
}

int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Homework #1", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
    glewInit();
#endif

	glViewport(0, 0, 640, 360);

	ShaderProgram program;

	program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");	

	GLuint player1Texture = LoadTexture(RESOURCE_FOLDER"assets/base_pack/Player/p1_front.png");
	GLuint player2Texture = LoadTexture(RESOURCE_FOLDER"assets/base_pack/Player/p2_front.png");
	GLuint enemyTexture = LoadTexture(RESOURCE_FOLDER"assets/base_pack/Enemies/slimeWalk1.png");
	GLuint item1Texture = LoadTexture(RESOURCE_FOLDER"assets/base_pack/Items/star.png");
	GLuint item2Texture = LoadTexture(RESOURCE_FOLDER"assets/base_pack/Items/fireball.png");
	GLuint tile1Texture = LoadTexture(RESOURCE_FOLDER"assets/base_pack/Tiles/grassCenter.png");
	GLuint tile2Texture = LoadTexture(RESOURCE_FOLDER"assets/base_pack/Tiles/dirtCenter.png");

	float tMap[4][4] = {
		0, 1, 1, 0,
		1, 0, 0, 1,
		1, 0, 0, 1,
		0, 1, 1, 0
	};

	float player1Pos_X = -0.5f;
	float player1Pos_Y = 0.5f;
	float player2Pos_X = 0.5f;
	float player2Pos_Y = 0.5f;
	float enemyPos_X = 0.0f;
	float enemyPos_Y = 0.0f;
	float item1Angle = 45.0f;
	float item2Angle = 315.0f;
	
	float leftBorder = -1.777f;
	float rightBorder = 1.777f;
	float topBorder = 1.0f;
	float bottomBorder = -1.0f;

	float lastFrameTicks = 0.0f;

	glm::mat4 projectionMatrix = glm::mat4(1.0f);
	glm::mat4 modelMatrix = glm::mat4(1.0f);
	glm::mat4 modelMatrix_p1 = glm::mat4(1.0f);
	glm::mat4 modelMatrix_p2 = glm::mat4(1.0f);
	glm::mat4 modelMatrix_enemy = glm::mat4(1.0f);
	glm::mat4 modelMatrix_item = glm::mat4(1.0f);
	glm::mat4 modelMatrix_tile = glm::mat4(1.0f);
	glm::mat4 viewMatrix = glm::mat4(1.0f);
	
	projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);

	glUseProgram(program.programID);

    SDL_Event event;
    bool done = false;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
        }

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		const Uint8 *keys = SDL_GetKeyboardState(NULL);
		
		// Player 1 movement and boundary checking
		if (keys[SDL_SCANCODE_LEFT] && canMove(player1Pos_X, leftBorder, "left"))
			player1Pos_X -= 0.5f * elapsed;
		else if (keys[SDL_SCANCODE_RIGHT] && canMove(player1Pos_X, rightBorder, "right"))
			player1Pos_X += 0.5f * elapsed;
		if (keys[SDL_SCANCODE_UP] && canMove(player1Pos_Y, topBorder, "up"))
			player1Pos_Y += 0.5f * elapsed;
		else if (keys[SDL_SCANCODE_DOWN] && canMove(player1Pos_Y, bottomBorder, "down"))
			player1Pos_Y -= 0.5f * elapsed;

		// Player 2 movement and boundary checking
		if (keys[SDL_SCANCODE_A] && canMove(player2Pos_X, leftBorder, "left"))
			player2Pos_X -= 0.5f * elapsed;
		else if (keys[SDL_SCANCODE_D] && canMove(player2Pos_X, rightBorder, "right"))
			player2Pos_X += 0.5f * elapsed;
		if (keys[SDL_SCANCODE_W] && canMove(player2Pos_Y, topBorder, "up"))
			player2Pos_Y += 0.5f * elapsed;
		else if (keys[SDL_SCANCODE_S] && canMove(player2Pos_Y, bottomBorder, "down"))
			player2Pos_Y -= 0.5f * elapsed;

		// Press 1 or 2 to speed up player items respectively
		item1Angle -= elapsed * 90.0f * (3.1415926f / 180.0f);
		item2Angle += elapsed * 90.0f * (3.1415926f / 180.0f);
		if (keys[SDL_SCANCODE_1]) 
			item1Angle -= elapsed * 90.0f * (3.1415926f / 180.0f);
		if (keys[SDL_SCANCODE_2])
			item2Angle += elapsed * 90.0f * (3.1415926f / 180.0f);

		// Press SPACE for random enemy movement
		if (keys[SDL_SCANCODE_SPACE]) {
			float randomX = (static_cast <float> (rand()) * 40.0f / static_cast <float> (RAND_MAX)) - 20.0f;
			float randomY = (static_cast <float> (rand()) * 40.0f / static_cast <float> (RAND_MAX)) - 20.0f;
			if (randomX < 0 && canMove(enemyPos_X, leftBorder, "left"))
				enemyPos_X += randomX * elapsed;
			if (randomY > 0 && canMove(enemyPos_Y, topBorder, "up"))
				enemyPos_Y += randomY * elapsed;
			if (randomX > 0 && canMove(enemyPos_X, rightBorder, "right"))
				enemyPos_X += randomX * elapsed;
			if (randomY < 0 && canMove(enemyPos_Y, bottomBorder, "down"))
				enemyPos_Y += randomY * elapsed;
		}

		glClear(GL_COLOR_BUFFER_BIT);

		program.SetProjectionMatrix(projectionMatrix);
		program.SetViewMatrix(viewMatrix);

		// Drawing Tiles
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				modelMatrix_tile = glm::translate(glm::mat4(1.0f), glm::vec3(i * 0.5f, j * 0.5f, 0.0f));
				modelMatrix_tile = glm::translate(modelMatrix_tile, glm::vec3(-0.75f, -0.75f, 0.0f));
				program.SetModelMatrix(modelMatrix_tile);
				
				if (tMap[i][j] == 0)
					glBindTexture(GL_TEXTURE_2D, tile1Texture);
				else
					glBindTexture(GL_TEXTURE_2D, tile2Texture);
				
				float tileVertices[] = { -0.25f, -0.25f, 0.25f, -0.25f, 0.25f, 0.25f, -0.25f, -0.25f, 0.25f, 0.25f, -0.25f, 0.25f };

				glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, tileVertices);
				glEnableVertexAttribArray(program.positionAttribute);

				float tileTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
				glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, tileTexCoords);
				glEnableVertexAttribArray(program.texCoordAttribute);

				glDrawArrays(GL_TRIANGLES, 0, 6);

				glDisableVertexAttribArray(program.positionAttribute);
				glDisableVertexAttribArray(program.texCoordAttribute);
			}		}

		// Drawing Player 1
		modelMatrix_p1 = glm::translate(glm::mat4(1.0f), glm::vec3(player1Pos_X, player1Pos_Y, 0.0f));
		program.SetModelMatrix(modelMatrix_p1);

		glBindTexture(GL_TEXTURE_2D, player1Texture);
		
		float player1Vertices[] = { -0.1f, -0.1f, 0.1f, -0.1f, 0.1f, 0.1f, -0.1f, -0.1f, 0.1f, 0.1f, -0.1f, 0.1f };
		
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, player1Vertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float player1TexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, player1TexCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		// Drawing Player 1's Item
		modelMatrix_item = glm::translate(glm::mat4(1.0f), glm::vec3(player1Pos_X, player1Pos_Y, 0.0f));
		modelMatrix_item = glm::rotate(modelMatrix_item, item1Angle, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix_item = glm::translate(modelMatrix_item, glm::vec3(0.0f, 0.5f, 0.0f));
		program.SetModelMatrix(modelMatrix_item);

		glBindTexture(GL_TEXTURE_2D, item1Texture);

		float item1Vertices[] = { -0.1f, -0.1f, 0.1f, -0.1f, 0.1f, 0.1f, -0.1f, -0.1f, 0.1f, 0.1f, -0.1f, 0.1f };

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, item1Vertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float item1TexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, item1TexCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		// Drawing Player 2
		modelMatrix_p2 = glm::mat4(1.0f);
		modelMatrix_p2 = glm::translate(modelMatrix_p2, glm::vec3(player2Pos_X, player2Pos_Y, 0.0f));
		program.SetModelMatrix(modelMatrix_p2);

		glBindTexture(GL_TEXTURE_2D, player2Texture);

		float player2Vertices[] = { -0.1f, -0.1f, 0.1f, -0.1f, 0.1f, 0.1f, -0.1f, -0.1f, 0.1f, 0.1f, -0.1f, 0.1f };

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, player2Vertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float player2TexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, player2TexCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		// Drawing Player 2's Item
		modelMatrix_item = glm::translate(glm::mat4(1.0f), glm::vec3(player2Pos_X, player2Pos_Y, 0.0f));
		modelMatrix_item = glm::rotate(modelMatrix_item, item2Angle, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix_item = glm::translate(modelMatrix_item, glm::vec3(0.0f, 0.5f, 0.0f));
		program.SetModelMatrix(modelMatrix_item);

		glBindTexture(GL_TEXTURE_2D, item2Texture);

		float item2Vertices[] = { -0.1f, -0.1f, 0.1f, -0.1f, 0.1f, 0.1f, -0.1f, -0.1f, 0.1f, 0.1f, -0.1f, 0.1f };

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, item2Vertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float item2TexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, item2TexCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		// Drawing Enemy
		modelMatrix_enemy = glm::mat4(1.0f);
		modelMatrix_enemy = glm::translate(modelMatrix_enemy, glm::vec3(enemyPos_X, enemyPos_Y, 0.0f));
		program.SetModelMatrix(modelMatrix_enemy);

		glBindTexture(GL_TEXTURE_2D, enemyTexture);

		float enemyVertices[] = { -0.1f, -0.1f, 0.1f, -0.1f, 0.1f, 0.1f, -0.1f, -0.1f, 0.1f, 0.1f, -0.1f, 0.1f };

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, enemyVertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float enemyTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, enemyTexCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		
		// Drawing Untextured Polygon(s)
		modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
		program.SetModelMatrix(modelMatrix);
		float triangleVertices[] = { 0.125f, -0.125f, 0.0f, 0.125f, -0.125f, -0.125f };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, triangleVertices);
		glEnableVertexAttribArray(program.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glDisableVertexAttribArray(program.positionAttribute);
		
		SDL_GL_SwapWindow(displayWindow);
    }

    SDL_Quit();
    return 0;
}