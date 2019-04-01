/*

	Homework #3 - Space Invaders
	jc7699@nyu.edu

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
#include "MainMenu.h"
#include "GameLevel.h"

using namespace std;

SDL_Window* displayWindow;
ShaderProgram texturedProgram;
SDL_Event event;

glm::mat4 projectionMatrix;
glm::mat4 modelMatrix;
glm::mat4 viewMatrix;
/*
#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6
float accumulator = 0.0f; 
*/
bool done = false;
float elapsed;
float lastFrameTicks;

#define MAX_ENEMIES 12
#define MAX_BULLETS 30
int bulletIndex = 0;

const int idleAnimation[] = { 0, 1, 2, 3 };
const int moveAnimation[] = { 4, 5, 6, 7, 8, 9 };
const int kickAnimation[] = { 10, 11, 12 };
const int hurtAnimation[] = { 13, 14, 15, 16 };
const int numFrames[] = { 4, 6, 3, 4 };
float animationElapsed = 0.0f;
float framesPerSecond = 10.0f;
int currentIndex[] = { 0, 0, 0, 0 };

float enemyDelay = 0.0f;
float bulletDelay = 0.0f;

GLuint fontTexture, playerTexture, enemyTexture, bulletTexture;

enum GameMode { STATE_MAIN_MENU, STATE_GAME_LEVEL, STATE_GAME_OVER, STATE_GAME_WIN };
GameMode mode;
GameState state;

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

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBindTexture(GL_TEXTURE_2D, fontTexture);

	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program.positionAttribute);

	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program.texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, text.size() * 6);

	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);
}

void shootBullet(Entity e) {
	state.bullets[bulletIndex].position[0] = e.position[0];
	state.bullets[bulletIndex].position[1] = e.position[1];
	state.bullets[bulletIndex].velocity[1] = 1.0f;
	bulletIndex++;
	if (bulletIndex > MAX_BULLETS + 1)
		bulletIndex = 0;
}

bool boxBoxCollision(Entity e1, Entity e2) {
	return fabs(e1.position[0] - e2.position[0]) - (e2.size[0] / 2 + e2.size[0] / 2) < 0 && fabs(e1.position[1] - e2.position[1]) - (e2.size[0] / 2 + e2.size[0] / 2) < 0;
}

void RenderMainMenu() {
	GLuint fontTexture = LoadTexture(RESOURCE_FOLDER"assets/font1.png");

	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.55f, 0.5f, 1.0f));
	texturedProgram.SetModelMatrix(modelMatrix);
	DrawText(texturedProgram, fontTexture, "SPACE", 0.2f, 0.1f);

	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.95f, 0.25f, 1.0f));
	texturedProgram.SetModelMatrix(modelMatrix);
	DrawText(texturedProgram, fontTexture, "INVADERS", 0.2f, 0.1f);

	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.65f, -0.75f, 1.0f));
	texturedProgram.SetModelMatrix(modelMatrix);
	DrawText(texturedProgram, fontTexture, "Press SPACE to play", 0.1f, -0.025f);
}

void RenderGameLevel(GameState& state) {
	animationElapsed += elapsed;
	if (animationElapsed > 1.0 / framesPerSecond) {
		animationElapsed = 0.0;
		for (int i = 0; i < 4; ++i) {
			currentIndex[i]++;
			if (currentIndex[i] > numFrames[i] - 1) 
				currentIndex[i] = 0;
		}
	}

	//deathAnimationTimer += elapsed;

	if (state.player.velocity[0] == 0)
		state.player.Draw(texturedProgram, idleAnimation[currentIndex[0]], 24, 1);
	else
		state.player.Draw(texturedProgram, moveAnimation[currentIndex[1]], 24, 1);

	for (int i = 0; i < MAX_ENEMIES; ++i) {
		if (state.enemies[i].health)
			state.enemies[i].Draw(texturedProgram, idleAnimation[currentIndex[0]], 24, 1);
	}

	for (int i = 0; i < MAX_BULLETS; ++i) 
		state.bullets[i].Draw(texturedProgram, 0, 1, 1);
}

void RenderGameWin() {
	GLuint fontTexture = LoadTexture(RESOURCE_FOLDER"assets/font1.png");

	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.2f, 0.5f, 1.0f));
	texturedProgram.SetModelMatrix(modelMatrix);
	DrawText(texturedProgram, fontTexture, "YOU", 0.2f, 0.0f);

	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.2f, 0.25f, 1.0f));
	texturedProgram.SetModelMatrix(modelMatrix);
	DrawText(texturedProgram, fontTexture, "WIN", 0.2f, 0.0f);
}

void RenderGameOver() {
	GLuint fontTexture = LoadTexture(RESOURCE_FOLDER"assets/font1.png");

	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.25f, 0.5f, 1.0f));
	texturedProgram.SetModelMatrix(modelMatrix);
	DrawText(texturedProgram, fontTexture, "GAME", 0.2f, 0.0f);

	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.25f, 0.25f, 1.0f));
	texturedProgram.SetModelMatrix(modelMatrix);
	DrawText(texturedProgram, fontTexture, "OVER", 0.2f, 0.0f);
}

void UpdateMainMenu(float elapsed) {
	// Nothing needed at the moment
	// May add interactive feature later
}

void UpdateGameLevel(GameState& state, float elapsed) {
	/*
	if (enemyDelay > 3.0f) {
		int randomEnemy = rand() % MAX_ENEMIES;
		if (state.enemies[randomEnemy].health > 0)
			shootBullet(state.enemies[randomEnemy]);
		enemyDelay = 0.0f;
	}
	*/
	state.player.position[0] += state.player.velocity[0] * elapsed;
	for (int i = 0; i < MAX_BULLETS; ++i) {
		state.bullets[i].position[1] += state.bullets[i].velocity[1] * elapsed;
		if (state.bullets[i].position[1] > 1.0f) {
			state.bullets[i].velocity[1] = 0.0f;
			state.bullets[i].position[0] = -2000.0f;
			state.bullets[i].position[1] = 0.0f;
		}
	}
	
	for (int i = 0; i < MAX_ENEMIES; ++i) {
		for (int j = 0; j < MAX_BULLETS; ++j) {
			if (boxBoxCollision(state.enemies[i], state.bullets[j]) && state.enemies[i].health > 0) {
				//glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
				state.enemies[i].health = 0.0f;
				state.bullets[j].velocity[1] = 0.0f;
				state.bullets[j].position[0] = -2000.0f;
				state.bullets[j].position[1] = 0.0f;
			}
		}
	}
	
	if (state.player.health == 0)
		mode = STATE_GAME_OVER;
	bool allDead = true;
	for (int i = 0; i < MAX_ENEMIES; ++i) {
		if (state.enemies[i].health > 0)
			allDead = false;
	}
	if (allDead)
		mode = STATE_GAME_WIN;
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
			if (event.key.keysym.scancode == SDL_SCANCODE_SPACE && bulletDelay > 1.0f) {
				shootBullet(state.player);
				bulletDelay = 0.0f;
			}
			else if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
				mode = STATE_MAIN_MENU;
		}
	}
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	if (keys[SDL_SCANCODE_LEFT] && state.player.position[0] > -1.677f)
		state.player.velocity.x = -1.0f;
	else if (keys[SDL_SCANCODE_RIGHT] && state.player.position[0] < 1.677f)
		state.player.velocity.x = 1.0f;
	else
		state.player.velocity.x = 0.0f;
}

void ProcessGameWinInput() {
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
			done = true;
		}
		else if (event.type == SDL_KEYDOWN)
			if (event.key.keysym.scancode == SDL_SCANCODE_SPACE)
				mode = STATE_GAME_LEVEL;
	}
}

void ProcessGameOverInput() {
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
			done = true;
		}
		else if (event.type == SDL_KEYDOWN)
			if (event.key.keysym.scancode == SDL_SCANCODE_SPACE)
				mode = STATE_GAME_LEVEL;
	}
}

void Render() {
	glClear(GL_COLOR_BUFFER_BIT);

	switch (mode) {
	case STATE_MAIN_MENU:
		RenderMainMenu();
		break;
	case STATE_GAME_LEVEL:
		RenderGameLevel(state);
		break;
	case STATE_GAME_WIN:
		RenderGameWin();
		break;
	case STATE_GAME_OVER:
		RenderGameOver();
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
	case STATE_GAME_WIN:
		ProcessGameWinInput();
		break;
	case STATE_GAME_OVER:
		ProcessGameOverInput();
		break;
	}
}

void Setup() {
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Space Invaders", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL);
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
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	mode = STATE_MAIN_MENU;
	srand(SDL_GetTicks());
}

int main(int argc, char *argv[])
{
	Setup();
	
	GLuint playerTexture = LoadTexture(RESOURCE_FOLDER"assets/vita.png");
	GLuint enemyTexture = LoadTexture(RESOURCE_FOLDER"assets/doux.png");
	GLuint bulletTexture = LoadTexture(RESOURCE_FOLDER"assets/bullet.png");

	//SheetSprite fontSprite = SheetSprite(fontTexture, 0.0f / 512.0f, 0.0f / 512.0f, 32.0f / 512.0f, 32.0f / 512.0f, 0.2f);
 	SheetSprite playerSprite = SheetSprite(playerTexture, 0.0f / 576.0f, 0.0f / 24.0f, 24.0f / 576.0f, 24.0f / 24.0f, 0.2f);
	SheetSprite enemySprite = SheetSprite(enemyTexture, 0.0f / 576.0f, 0.0f / 24.0f, 24.0f / 576.0f, 24.0f / 24.0f, 0.2f);
	SheetSprite bulletSprite = SheetSprite(bulletTexture, 0.0f / 22.0f, 0.0f / 22.0f, 22.0f / 22.0f, 22.0f / 22.0f, 0.1f);

	glm::vec3 playerPosition(0.0f, -0.75f, 1.0f);
	glm::vec3 playerVelocity(0.0f, 0.0f, 0.0f);
	glm::vec3 playerSize(1.0f, 1.0f, 1.0f);
	
	float enemyBox_x = -0.75f;
	float enemyBox_y = 0.75;
	glm::vec3 enemyPosition(enemyBox_x, enemyBox_y, 1.0f);
	glm::vec3 enemyVelocity(0.0f, 0.0f, 0.0f);
	glm::vec3 enemySize(1.0f, 1.0f, 1.0f);

	glm::vec3 bulletPosition(-2000.0f, 0.0f, 1.0f);
	glm::vec3 bulletVelocity(0.0f, 0.0f, 0.0f);
	glm::vec3 bulletSize(0.1f, 0.1f, 1.0f);

	Entity player = Entity(playerPosition, playerVelocity, playerSize, 0.0f, playerSprite);
	state.player = player;
	for (int i = 0; i < MAX_ENEMIES; ++i) {
		state.enemies[i] = Entity(enemyPosition, enemyVelocity, enemySize, 0.0f, enemySprite);
		enemyBox_x += 0.5f;
		if (enemyBox_x >= 1.25f) {
			enemyBox_y -= 0.25f;
			enemyBox_x = -0.75f;
		}
		enemyPosition = glm::vec3(enemyBox_x, enemyBox_y, 1.0f);
	}
	for (int i = 0; i < MAX_BULLETS; ++i)
		state.bullets[i] = Entity(bulletPosition, bulletVelocity, bulletSize, 0.0f, bulletSprite);

    while (!done) {
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		bulletDelay += elapsed;
		enemyDelay += elapsed;

		/* //May add back in later to fix physics bugs
		elapsed += accumulator;
		if (elapsed < FIXED_TIMESTEP) {
			accumulator = elapsed;
			continue;
		}
		while (elapsed >= FIXED_TIMESTEP) {
			Update(FIXED_TIMESTEP);
			elapsed -= FIXED_TIMESTEP;
		}
		accumulator = elapsed;
		*/

		ProcessInput();
		Update(elapsed);
		Render();
    }
    SDL_Quit();
    return 0;
}