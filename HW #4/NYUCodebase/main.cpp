/*

	Homework #4 - Simple 2D Platfomer Demo
	jc7699@nyu.edu

	CONTROLS: 
	LEFT and RIGHT to move player
	SPACE to start game from main menu
	ESC to return to main menu

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
#include "GameLevel.h"

using namespace std;

// Globl game variables
#define MAX_ENEMIES 3
#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6

#define TILE_SIZE 0.2f
#define SPRITE_COUNT_X 8
#define SPRITE_COUNT_Y 12
#define LEVEL_HEIGHT 16
#define LEVEL_WIDTH 22

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

const int idleAnimation[] = { 0, 1, 2, 3 };
const int moveAnimation[] = { 4, 5, 6, 7, 8, 9 };
const int numFrames[] = { 4, 6 };
float animationElapsed = 0.0f;
float framesPerSecond = 50.0f;
int currentIndex[] = { 0, 0 };

glm::vec2 friction = glm::vec2(0.1f, 0.1f);

GLuint fontTexture, playerTexture, enemyTexture, tileMapTexture;
SheetSprite playerSprite, enemySprite;

enum GameMode { STATE_MAIN_MENU, STATE_GAME_LEVEL };
GameMode mode;
GameState state;

int mapHeight, mapWidth;
unsigned int** mapData;
unsigned int levelData[LEVEL_HEIGHT][LEVEL_WIDTH];

Entity tiles[LEVEL_HEIGHT][LEVEL_WIDTH];

float lerp(float v0, float v1, float t) {
	return (1.0f - t) * v0 + t * v1;
}

void placeEntity(string type, float x, float y) {
	if (type == "Player") {
		state.player = new Entity(ENTITY_PLAYER, false, playerSprite);
		state.player->position.x = x;
		state.player->position.y = y;
	}
	else if (type == "Enemy") {
		Entity* enemy = new Entity(ENTITY_ENEMY, false, enemySprite);
		enemy->position.x = x;
		enemy->position.y = y;
		enemy->acceleration.x = 0.5f;
		state.enemies.push_back(enemy);
	}
}

bool readHeader(ifstream &stream) {
	string line;
	mapWidth = -1;
	mapHeight = -1;
	while
		(getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "width")
			mapWidth = atoi(value.c_str());
		else if (key == "height")
			mapHeight = atoi(value.c_str());
	}
	if (mapWidth == -1 || mapHeight == -1)
		return false;
	else { // allocate our map data
		mapData = new unsigned int*[mapHeight];
		for (int i = 0; i < mapHeight; ++i) {
			mapData[i] = new unsigned int[mapWidth];
		}
		return true;
	}
}

bool readLayerData(ifstream& stream) {
	string line;
	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "data") {
			for (int y = 0; y < mapHeight; y++) {
				getline(stream, line);
				istringstream lineStream(line);
				string tile;
				for (int x = 0; x < mapWidth; x++) {
					getline(lineStream, tile, ',');
					unsigned int val = (unsigned int)atoi(tile.c_str());
					if (val > 0) {
						// be careful, the tiles in this format are indexed from 1 not 0
						mapData[y][x] = val - 1;
					}
					else {
						mapData[y][x] = 0;
					}
				}
			}
		}
	}
	return true;
}

bool readEntityData(ifstream& stream) {
	string line;
	string type;
	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "type") {
			type = value;
		}
		else if (key == "location") {
			istringstream lineStream(value);
			string xPosition, yPosition;
			getline(lineStream, xPosition, ',');
			getline(lineStream, yPosition, ',');
			float placeX = atoi(xPosition.c_str()) * TILE_SIZE;
			float placeY = atoi(yPosition.c_str()) * -TILE_SIZE;
			placeEntity(type, placeX, placeY);
		}
	}
	return true;
}

void readMapData() {
	ifstream infile(RESOURCE_FOLDER"TileMap.txt");
	string line;
	while (getline(infile, line)) {
		if (line == "[header]") {
			if (!readHeader(infile)) {
				return;
			}
		}
		else if (line == "[layer]") {
			readLayerData(infile);
		}
		else if (line == "[ObjectsLayer]") {
			readEntityData(infile);
		}
	}
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

void DrawTileMap(ShaderProgram& program, int tileMapTexture) {
	modelMatrix = glm::mat4(1.0f);
	program.SetModelMatrix(modelMatrix);
	
	std::vector<float> vertexData;
	std::vector<float> texCoordData;

	for (int y = 0; y < LEVEL_HEIGHT; y++) {
		for (int x = 0; x < LEVEL_WIDTH; x++) {
			if (mapData[y][x] != 0) {
				float u = (float)(((int)mapData[y][x]) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
				float v = (float)(((int)mapData[y][x]) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;

				float spriteWidth = 1.0f / (float)SPRITE_COUNT_X;
				float spriteHeight = 1.0f / (float)SPRITE_COUNT_Y;

				vertexData.insert(vertexData.end(), {
					TILE_SIZE * x, -TILE_SIZE * y,
					TILE_SIZE * x, (-TILE_SIZE * y) - TILE_SIZE,
					(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
					
					TILE_SIZE * x, -TILE_SIZE * y,
					(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
					(TILE_SIZE * x) + TILE_SIZE, -TILE_SIZE * y
				});

				texCoordData.insert(texCoordData.end(), {
					u, v,
					u, v + (spriteHeight),
					u + spriteWidth, v + (spriteHeight),

					u, v,
					u + spriteWidth, v + (spriteHeight),
					u + spriteWidth, v
				});
			}
		}
	}
	
	glBindTexture(GL_TEXTURE_2D, tileMapTexture);

	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program.positionAttribute);

	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program.texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, LEVEL_HEIGHT * LEVEL_WIDTH * 6);

	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);
}

void worldToTileCoordinates(float worldX, float worldY, int *gridX, int *gridY) {
	*gridX = (int)(worldX / TILE_SIZE);
	*gridY = (int)(worldY / -TILE_SIZE);
}

void resetEntities() {
	// Nothing implemented here so far
}

void RenderMainMenu() {
	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.25f, 0.5f, 1.0f));
	texturedProgram.SetModelMatrix(modelMatrix);
	DrawText(texturedProgram, fontTexture, "DINO", 0.2f, 0.0f);

	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.85f, 0.25f, 1.0f));
	texturedProgram.SetModelMatrix(modelMatrix);
	DrawText(texturedProgram, fontTexture, "PLATFORMER", 0.2f, 0.0f);

	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.85f, -0.75f, 1.0f));
	texturedProgram.SetModelMatrix(modelMatrix);
	DrawText(texturedProgram, fontTexture, "Press SPACE to play", 0.1f, 0.0f);
}

void RenderGameLevel(GameState& state) {
	DrawTileMap(texturedProgram, tileMapTexture);
	state.player->Draw(texturedProgram, idleAnimation[currentIndex[0]]);
	viewMatrix = glm::mat4(1.0f);
	viewMatrix = glm::translate(viewMatrix, glm::vec3(-state.player->position.x, -state.player->position.y, -state.player->position.z));
	texturedProgram.SetViewMatrix(viewMatrix);
	for (int i = 0; i < MAX_ENEMIES; ++i) {
		if (!state.enemies[i]->isCaught)
			state.enemies[i]->Draw(texturedProgram, idleAnimation[currentIndex[0]]);
	}
}

void UpdateMainMenu(float elapsed) {
	// Nothing needed at the moment for a simple main menu
	// May add time-related elements (animations) if necessary
}

void UpdateGameLevel(GameState& state, float elapsed) {
	state.player->resetCollisionFlags();
	state.player->velocity.x = lerp(state.player->velocity.x, 0.0f, elapsed * friction.x);
	state.player->velocity.y = lerp(state.player->velocity.y, 0.0f, elapsed * friction.y);

	state.player->velocity.x += state.player->acceleration.x * elapsed;
	state.player->velocity.y += state.player->acceleration.y * elapsed;
	
	int gridX = -1;
	int gridY = -1;
	worldToTileCoordinates(state.player->position.x, state.player->position.y, &gridX, &gridY);

	if (gridX > -1 && gridX < 22 && gridY > -1 && gridY << 16) {
		state.player->position.y += state.player->velocity.y * elapsed;
		if (mapData[gridY][gridX] != 0) {
			state.player->collisionY(tiles[gridY][gridX], -TILE_SIZE * gridY);
		}

		state.player->position.x += state.player->velocity.x * elapsed;
		if (mapData[gridY][gridX] != 0) {
			state.player->collisionX(tiles[gridY][gridX], TILE_SIZE * gridX);
		}
	}

	int numCaught = 0;
	for (Entity* enemy : state.enemies) {
		if (!enemy->isCaught) {
			enemy->velocity.x += enemy->acceleration.x * elapsed;
			enemy->velocity.y += enemy->acceleration.y * elapsed;

			int gridX = -1;
			int gridY = -1;
			worldToTileCoordinates(enemy->position.x, enemy->position.y, &gridX, &gridY);

			if (gridX > -1 && gridX < 22 && gridY > -1 && gridY << 16) {
				enemy->position.y += enemy->velocity.y * elapsed;
				if (mapData[gridY][gridX] != 0) {
					enemy->collisionY(tiles[gridY][gridX], -TILE_SIZE * gridY);
				}

				enemy->position.x += state.player->velocity.x * elapsed;
				if (mapData[gridY][gridX] != 0) {
					if (enemy->collisionX(tiles[gridY][gridX], TILE_SIZE * gridX))
						enemy->velocity.x = -enemy->velocity.x;
				}
			}
			if (enemy->collidesWith(*state.player)) {
				enemy->isCaught = true;
				numCaught++;
			}
		}
	}

	if (numCaught == 3) {
		worldToTileCoordinates(state.player->position.x, state.player->position.y, &gridX, &gridY);
		if (gridX > -1 && gridX < 22 && gridY > -1 && gridY << 16) {
			if (mapData[gridY][gridX] == 29) {
				mode = STATE_MAIN_MENU;
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
			if (event.key.keysym.scancode == SDL_SCANCODE_SPACE && state.player->collidedBottom) {
				state.player->velocity.y = 1.0f;
			}
		}
	}
	const Uint8 *keys = SDL_GetKeyboardState(NULL);

	state.player->acceleration.x = 0.0f;

	if (keys[SDL_SCANCODE_LEFT] && state.player->position.x > 0.0f)
		state.player->acceleration.x = -0.5f;
	else if (keys[SDL_SCANCODE_RIGHT] && state.player->position.x < TILE_SIZE * LEVEL_WIDTH)
		state.player->acceleration.x = 0.5f;
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
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	mode = STATE_MAIN_MENU;
	fontTexture = LoadTexture(RESOURCE_FOLDER"assets/font.png");
	playerTexture = LoadTexture(RESOURCE_FOLDER"assets/vita.png");
	enemyTexture = LoadTexture(RESOURCE_FOLDER"assets/doux.png");
	tileMapTexture = LoadTexture(RESOURCE_FOLDER"assets/cavesofgallet_tiles.png");

	playerSprite = SheetSprite(playerTexture, 24 ,1);
	enemySprite = SheetSprite(enemyTexture, 24, 1);
	readMapData();
	
	for (int y = 0; y < LEVEL_HEIGHT; y++) {
		for (int x = 0; x < LEVEL_WIDTH; x++) {
			if (mapData[y][x] != 0) {
				Entity tile = Entity();
				tile.position = glm::vec3(x * TILE_SIZE, -y * TILE_SIZE, 0.0f);
				tiles[y][x] = tile;
			}
		}
	}

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
		animationElapsed += elapsed;
		
		if (animationElapsed > 1.0 / framesPerSecond) {
			for (int i = 0; i < 2; ++i) {
				currentIndex[i]++;
				animationElapsed = 0.0;
				if (currentIndex[i] > numFrames[i] - 1)
					currentIndex[i] = 0;
			}
		}
		
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