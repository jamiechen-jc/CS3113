/*

	SAT Collision Demo
	jc7699@nyu.edu

	Press SPACE to start
	Press ESCAPE to return to menu

*/

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
#include <vector>

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

#include "SheetSprite.h"
#include "Entity.h"
#include "GameState.h"
#include "SatCollision.h"

using namespace std;


#define MAX_ENEMIES 6
#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6

SDL_Window* displayWindow;
ShaderProgram texturedProgram;
SDL_Event event;

glm::mat4 projectionMatrix;
glm::mat4 modelMatrix;
glm::mat4 viewMatrix;

bool done = false;
float elapsed;
float lastFrameTicks;
float accumulator = 0.0f;

glm::vec2 friction = glm::vec2(0.1f, 0.1f);

GLuint fontTexture, spriteTexture;
SheetSprite playerSprite, enemySprite;

enum GameMode { STATE_MAIN_MENU, STATE_GAME_LEVEL };
GameMode mode;
GameState state;

float lerp(float v0, float v1, float t) {
	return (1.0f - t) * v0 + t * v1;
}

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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	stbi_image_free(image);
	return retTexture;
}

void DrawText(ShaderProgram &program, int fontTexture, std::string text, float size, float spacing) {
	float character_size = 1.0 / 16.0f;
	std::vector<float> vertexData;
	std::vector<float> texCoordData;

	for (size_t i = 0; i < text.size(); i++) {
		int spriteIndex = (int)text[i];
		float texture_x = (float)(spriteIndex % 16) / 16.0f;
		float texture_y = (float)(spriteIndex / 16) / 16.0f;

		vertexData.insert(vertexData.end(), {
	 		((size + spacing) * i) + (-0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
		});
		texCoordData.insert(texCoordData.end(), {
			texture_x, texture_y,
			texture_x, texture_y + character_size,
			texture_x + character_size, texture_y,
			texture_x + character_size, texture_y + character_size,
			texture_x + character_size, texture_y,
			texture_x, texture_y + character_size,
		});
	}

	glBindTexture(GL_TEXTURE_2D, fontTexture);

	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program.positionAttribute);

	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program.texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, text.size() * 6);

	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);
}

void resetEntities() {
	state.player = new Entity(ENTITY_PLAYER, false, playerSprite);
	state.player->position = glm::vec3(1.0f, 0.5f, 0.0f);

	float startPos = -0.8f;
	state.enemies.clear();
	for (int i = 0; i < MAX_ENEMIES; ++i) {
		Entity* enemy = new Entity(ENTITY_ENEMY, false, enemySprite);
		enemy->position.x = startPos;
		if (rand() % 2)
			enemy->velocity.x = -0.5f;
		else
			enemy->velocity.x = 0.5f;
		if (rand() % 2)
			enemy->velocity.y = -0.5f;
		else
			enemy->velocity.y = 0.5f;
		
		state.enemies.push_back(enemy);
		startPos += 0.4f;
	}
}

void RenderMainMenu() {
	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.25f, 0.5f, 1.0f));
	texturedProgram.SetModelMatrix(modelMatrix);
	DrawText(texturedProgram, fontTexture, "SAT", 0.2f, 0.0f);

	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.85f, 0.25f, 1.0f));
	texturedProgram.SetModelMatrix(modelMatrix);
	DrawText(texturedProgram, fontTexture, "COLLISIONS", 0.2f, 0.0f);

	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.85f, -0.75f, 1.0f));
	texturedProgram.SetModelMatrix(modelMatrix);
	DrawText(texturedProgram, fontTexture, "Press SPACE to start", 0.1f, 0.0f);
}

void RenderGameLevel(GameState& state) {
	state.player->Draw(texturedProgram, 35);
	/* // Code to make camera follow player
	viewMatrix = glm::mat4(1.0f);
	viewMatrix = glm::mat4(1.0f);
	viewMatrix = glm::translate(viewMatrix, glm::vec3(-state.player->position.x, -state.player->position.y, -state.player->position.z));
	texturedProgram.SetViewMatrix(viewMatrix);
	*/
	for (int i = 0; i < MAX_ENEMIES; ++i) {
		if (i % 2)
			state.enemies[i]->Draw(texturedProgram, 3);
		else 
			state.enemies[i]->Draw(texturedProgram, 19);
	}
}

void UpdateMainMenu(float elapsed) {
}

void UpdateGameLevel(GameState& state, float elapsed) {
	state.player->resetCollisionFlags();
	state.player->velocity.x = lerp(state.player->velocity.x, 0.0f, elapsed * friction.x);
	state.player->velocity.y = lerp(state.player->velocity.y, 0.0f, elapsed * friction.y);

	state.player->velocity.x += state.player->acceleration.x * elapsed;
	state.player->velocity.y += state.player->acceleration.y * elapsed;

	state.player->position.x += state.player->velocity.x * elapsed;
	state.player->position.y += state.player->velocity.y * elapsed;
	
	for (int i = 0; i < MAX_ENEMIES; ++i) {
		state.enemies[i]->resetCollisionFlags();
		//state.enemies[i]->velocity.x = lerp(state.enemies[i]->velocity.x, 0.0f, elapsed * friction.x);
		//state.enemies[i]->velocity.y = lerp(state.enemies[i]->velocity.y, 0.0f, elapsed * friction.y);

		state.enemies[i]->velocity.x += state.enemies[i]->acceleration.x * elapsed;
		state.enemies[i]->velocity.y += state.enemies[i]->acceleration.y * elapsed;
		
		state.enemies[i]->position.x += state.enemies[i]->velocity.x * elapsed;
		state.enemies[i]->position.y += state.enemies[i]->velocity.y * elapsed;

		if (state.enemies[i]->position.x >= 1.777f - state.enemies[i]->size.x/2 || state.enemies[i]->position.x <= -1.777f + state.enemies[i]->size.x/2)
			state.enemies[i]->velocity.x = -state.enemies[i]->velocity.x;
		if (state.enemies[i]->position.y >= 1.0f - state.enemies[i]->size.y/2 || state.enemies[i]->position.y <= -1.0f + state.enemies[i]->size.y/2)
			state.enemies[i]->velocity.y = -state.enemies[i]->velocity.y;
	}

	vector<pair<float, float>> playerPoints = state.player->getPoints();
	vector<pair<float, float>> enemyPoints;
	pair<float, float> penetration;

	for (int i = 0; i < MAX_ENEMIES; ++i) {
		enemyPoints = state.enemies[i]->getPoints();
		if (CheckSATCollision(playerPoints, enemyPoints, penetration)) {
			state.player->position.x += penetration.first;
			state.player->position.y += penetration.second;
			state.enemies[i]->position.x -= penetration.first;
			state.enemies[i]->position.y -= penetration.second;
		}
		for (int j = 0; j < MAX_ENEMIES; ++j) {
			vector<pair<float, float>> otherEnemyPoints;
			if (j != i) {
				otherEnemyPoints = state.enemies[j]->getPoints();
				if (CheckSATCollision(enemyPoints, otherEnemyPoints, penetration)) {
					state.enemies[i]->position.x += penetration.first;
					state.enemies[i]->position.y += penetration.second;
					state.enemies[j]->position.x -= penetration.first;
					state.enemies[j]->position.y -= penetration.second;
				}
			}
		}
	}
}

void ProcessMainMenuInput() {
	while (SDL_PollEvent(&event))
		if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE)
			done = true;
		else if (event.type == SDL_KEYDOWN)
			if (event.key.keysym.scancode == SDL_SCANCODE_SPACE)
				mode = STATE_GAME_LEVEL;
}

void ProcessGameLevelInput(GameState& state) {
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE)
			done = true;
		else if (event.type == SDL_KEYDOWN) {
			if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
				mode = STATE_MAIN_MENU;
				resetEntities();
			}
		}
	}
	const Uint8 *keys = SDL_GetKeyboardState(NULL);

	state.player->acceleration.x = 0.0f;
	state.player->acceleration.y = 0.0f;

	if (keys[SDL_SCANCODE_LEFT] && state.player->position.x > -1.777f + state.player->size.x)
		state.player->acceleration.x = -0.5f;
	else if (keys[SDL_SCANCODE_RIGHT] && state.player->position.x < 1.777f - state.player->size.x)
		state.player->acceleration.x = 0.5f;
	else if (keys[SDL_SCANCODE_UP] && state.player->position.y < 1.0f - state.player->size.y)
		state.player->acceleration.y = 0.5f;
	else if (keys[SDL_SCANCODE_DOWN] && state.player->position.y > -1.0f + state.player->size.y)
		state.player->acceleration.y = -0.5f;
}

void Render() {
	glClearColor(0.1294f, 0.149f, 0.247f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	switch (mode) {
	case STATE_MAIN_MENU:
		RenderMainMenu();
		break;
	case STATE_GAME_LEVEL:
		RenderGameLevel(state);
		break;
	}
	SDL_GL_SwapWindow(displayWindow);
}
void Update(float elapsed) {
	switch (mode) {
	case STATE_MAIN_MENU:
		UpdateMainMenu(elapsed);
		break;
	case STATE_GAME_LEVEL:
		UpdateGameLevel(state, elapsed);
		break;
	}
}
void ProcessInput() {
	switch (mode) {
	case STATE_MAIN_MENU:
		ProcessMainMenuInput();
		break;
	case STATE_GAME_LEVEL:
		ProcessGameLevelInput(state);
		break;
	}
}

void Setup() {
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Platformer Demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
	glewInit();
#endif

	glViewport(0, 0, 1280, 720);
	
	texturedProgram.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	glm::mat4 viewMatrix = glm::mat4(1.0f);
	projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);

	texturedProgram.SetProjectionMatrix(projectionMatrix);
	texturedProgram.SetViewMatrix(viewMatrix);
	glUseProgram(texturedProgram.programID);
	/*
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	*/
	mode = STATE_MAIN_MENU;
	fontTexture = LoadTexture(RESOURCE_FOLDER"assets/font.png");
	spriteTexture = LoadTexture(RESOURCE_FOLDER"assets/NTlikeTDSSprites.png");

	playerSprite = SheetSprite(spriteTexture, 8, 17);
	enemySprite = SheetSprite(spriteTexture, 8, 17);

	resetEntities();
}

int main(int argc, char *argv[])
{
	Setup();

     while (!done) {
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		elapsed += accumulator;
		
		ProcessInput();

		if (elapsed < FIXED_TIMESTEP) {
			accumulator = elapsed;
			continue;
		}
		while (elapsed >= FIXED_TIMESTEP) {
			Update(FIXED_TIMESTEP);
			elapsed -= FIXED_TIMESTEP;
		}
		accumulator = elapsed;

  		Render();
    }
    SDL_Quit();
    return 0;
}