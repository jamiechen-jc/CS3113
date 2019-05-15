/*

	Final Project
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
#include <SDL_mixer.h>
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
#include "Helper.h"

using namespace std;

#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6

#define TILE_SIZE 0.5f
#define SPRITE_COUNT_X 86
#define SPRITE_COUNT_Y 56
#define LEVEL_HEIGHT 25
#define LEVEL_WIDTH 100
#define BACKGROUND_LAYERS 3

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
const int moveAnimation[] = { 8, 9, 10, 11, 12, 13 };
const int jumpAnimation[] = { 16, 16, 16, 16, 17, 17, 17, 17 };
const int fallAnimation[] = { 22, 23 };
const int attackAnimation[] = { 44, 45, 46, 47, 48, 49, 50, 51, 52 };

const int idleEnemyAnimation[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

const int numFrames[] = { 4, 6, 10, 2, 9, 12 };
float animationElapsed = 0.0f;
float framesPerSecond[] = { 5.0f , 5.0f, 5.0f, 5.0f, 5.0f, 5.0f };
int currentIndex[] = { 0, 0, 0, 0, 0, 0 };

float hungerTimer = 0.0f;
float starvationTimer = 0.0f;
float quitTimer = 0.0f;

glm::vec2 friction = glm::vec2(0.75f, 0.0f);
//glm::vec4 camera_window;
glm::vec3 camera;

GLuint fontTexture, spriteTexture, enemyTexture, tileMapTexture, UITexture;
vector<GLuint> backgroundTexture;
SheetSprite playerSprite, enemySprite;

int mapHeight, mapWidth;
unsigned int** mapData;
unsigned int** backgroundData[BACKGROUND_LAYERS];
int currentBackgroundLayer = 0;
Entity* tiles[LEVEL_HEIGHT][LEVEL_WIDTH];

Mix_Music *menu_music;
Mix_Music *game_music;
Mix_Music *dungeon_music;
Mix_Chunk *footstep1;
Mix_Chunk *footstep2;

int currentLevel = 0;
enum GameMode { STATE_MAIN_MENU, STATE_GAME_LEVEL, STATE_GAME_OVER };
GameMode mode;
GameState state;

void placeEntity(string type, float x, float y) {
	if (type == "Player") {
		state.player->isAttacking = false;
		state.player->position.x = x;
		state.player->position.y = y;
	}
	else if (type == "Enemy") {
		Entity* enemy = new Entity(ENTITY_ENEMY, false, enemySprite);
		enemy->size = glm::vec3(0.5f, 0.5f, 1.0f);
		enemy->health = 10;
		enemy->position.x = x;
		enemy->position.y = y;
		enemy->isAttacking = true;

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

		for (int i = 0; i < BACKGROUND_LAYERS; ++i) {
			backgroundData[i] = new unsigned int*[mapHeight];
			for (int j = 0; j < mapHeight; ++j) {
				backgroundData[i][j] = new unsigned int[mapWidth];
			}
		}

		return true;
	}
}

bool readLayerData(ifstream& stream) {
	string line, type;
	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "type")
			type = value;
		else if (key == "data") {
			if (type == "TilesLayer") {
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

			else if (type == "BackgroundLayer") {
				for (int y = 0; y < mapHeight; y++) {
					getline(stream, line);
					istringstream lineStream(line);
					string tile;
					for (int x = 0; x < mapWidth; x++) {
						getline(lineStream, tile, ',');
						unsigned int val = (unsigned int)atoi(tile.c_str());
						if (val > 0) {
							// be careful, the tiles in this format are indexed from 1 not 0
							backgroundData[currentBackgroundLayer][y][x] = val - 1;
						}
						else {
							backgroundData[currentBackgroundLayer][y][x] = 0;
						}
					}
				}
				currentBackgroundLayer++;
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
	currentBackgroundLayer = 0;
	string filepath = "";
	
	if (currentLevel == 0)
		filepath = RESOURCE_FOLDER"levels/HomeLevel.txt";
	else if (currentLevel == 1)
		filepath = RESOURCE_FOLDER"levels/Level1.txt";
	else if (currentLevel == 2)
		filepath = RESOURCE_FOLDER"levels/Level2.txt";
	else if (currentLevel == 3)
		filepath = RESOURCE_FOLDER"levels/Level3.txt";

	ifstream infile(filepath);

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

void worldToTileCoordinates(float worldX, float worldY, int *gridX, int *gridY) {
	*gridX = (int)(worldX / TILE_SIZE);
	*gridY = (int)(worldY / -TILE_SIZE);
}

void drawTileMap(ShaderProgram& program, int tileMapTexture) {
	modelMatrix = glm::mat4(1.0f);
	program.SetModelMatrix(modelMatrix);

	std::vector<float> vertexData;
	std::vector<float> texCoordData;

	for (int y = 0; y < mapHeight; y++) {
		for (int x = 0; x < mapWidth; x++) {
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

	glDrawArrays(GL_TRIANGLES, 0, vertexData.size() / 2);

	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);
}

void drawUI(ShaderProgram& program, int UITexture) {

	// Drawing health bar

	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, state.player->position);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-5.0f, -3.5f, 1.0f));
	modelMatrix = glm::scale(modelMatrix, glm::vec3(10.0f, 2.5f, 1.0f));
	program.SetModelMatrix(modelMatrix);
	
	glBindTexture(GL_TEXTURE_2D, UITexture);
	
	float index = 12 - (state.player->health / 25) * 3;

	float u = (float)(((int)index) % 3) / 3;
	float v = (float)(((int)index) / 3) / 29;
	float spriteWidth = 1.0f / 3;
	float spriteHeight = 1.0f / 29;

	float texCoords[] = {
		u, v + spriteHeight,
		u + spriteWidth, v,
		u, v,
		u + spriteWidth, v,
		u, v + spriteHeight,
		u + spriteWidth, v + spriteHeight
	};

	float aspect = spriteHeight / spriteWidth;

	float vertices[] = {
		-0.5f * aspect, -0.5f * aspect,
		0.5f * aspect, 0.5f * aspect,
		-0.5f * aspect, 0.5f * aspect,
		0.5f * aspect, 0.5f * aspect,
		-0.5f * aspect, -0.5f * aspect,
		0.5f * aspect, -0.5f * aspect
	};

	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program.positionAttribute);

	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program.texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);

	// Drawing hunger bar
	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, state.player->position);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-3.0f, -3.5f, 1.0f));
	modelMatrix = glm::scale(modelMatrix, glm::vec3(10.0f, 2.5f, 1.0f));
	program.SetModelMatrix(modelMatrix);

	glBindTexture(GL_TEXTURE_2D, UITexture);

	index = 66 - (state.player->hunger / 25) * 3;

	u = (float)(((int)index) % 3) / (float)3;
	v = (float)(((int)index) / 3) / (float)29;

	float texCoords2[] = {
		u, v + spriteHeight,
		u + spriteWidth, v,
		u, v,
		u + spriteWidth, v,
		u, v + spriteHeight,
		u + spriteWidth, v + spriteHeight
	};

	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program.positionAttribute);

	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords2);
	glEnableVertexAttribArray(program.texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);

	// Drawing other UI elements
	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, state.player->position);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.0f, -3.5f, 1.0f));
	program.SetModelMatrix(modelMatrix);
	int gridX, gridY;
	worldToTileCoordinates(state.player->position.x, state.player->position.y - state.player->size.y / 2, &gridX, &gridY);
	DrawText(program, fontTexture, "Gold: " + to_string(state.player->gold) + "    Potions: " + to_string(state.player->potions) + "     Food: " + to_string(state.player->food) + "     Pos: " + to_string(gridX) + ", " + to_string(gridY), 0.25f, -0.1f);
}

bool changeLevel() {
	int gridX, gridY;
	worldToTileCoordinates(state.player->position.x, state.player->position.y - state.player->size.y / 2, &gridX, &gridY);

	if (currentLevel == 0 && gridX > 89 && gridX < 92 && gridY <= 15) {
		currentLevel = 1;
		return true;
	}
	else if (currentLevel == 1 && gridX == 3 && gridY == 6) {
		currentLevel = 0;
		return true;
	}
	else if (currentLevel == 1 && gridX == 43 && gridY == 24) {
		currentLevel = 2;
		return true;
	}
	else if (currentLevel == 2 && gridX == 25 && gridY == 6) {
		currentLevel = 1;
		return true;
	}
	else if (currentLevel == 2 && gridX == 21 && gridY == 24) {
		currentLevel = 3;
		return true;
	}
	else if (currentLevel == 3 && gridX == 46 && gridY == 24) {
		currentLevel = 2;
		return true;
	}
	
	return false;
}

void resetEntities() {
	state.player = new Entity(ENTITY_PLAYER, false, playerSprite);
	
	state.player->size = glm::vec3(2.0f, 2.0f, 1.0f);
	state.player->health = 100;
	state.player->hunger = 100;
	state.player->gold = 20;
	state.player->potions = 5;
	state.player->food = 5;
	
	readMapData();
}

void RenderMainMenu() {
	viewMatrix = glm::mat4(1.0f);
	projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);

	texturedProgram.SetViewMatrix(viewMatrix);
	texturedProgram.SetProjectionMatrix(projectionMatrix);
	
	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.25f, 0.5f, 1.0f));
	texturedProgram.SetModelMatrix(modelMatrix);
	DrawText(texturedProgram, fontTexture, "Final", 0.2f, -0.1f);

	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.35f, 0.25f, 1.0f));
	texturedProgram.SetModelMatrix(modelMatrix);
	DrawText(texturedProgram, fontTexture, "Project", 0.2f, -0.1f);

	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.85f, -0.75f, 1.0f));
	texturedProgram.SetModelMatrix(modelMatrix);
	DrawText(texturedProgram, fontTexture, "Press SPACE to start", 0.1f, -0.01f);
}

void RenderGameOver() {
	viewMatrix = glm::mat4(1.0f);
	projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);

	texturedProgram.SetViewMatrix(viewMatrix);
	texturedProgram.SetProjectionMatrix(projectionMatrix);

	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.25f, 0.5f, 1.0f));
	texturedProgram.SetModelMatrix(modelMatrix);
	DrawText(texturedProgram, fontTexture, "Game", 0.2f, -0.1f);

	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.25f, 0.25f, 1.0f));
	texturedProgram.SetModelMatrix(modelMatrix);
	DrawText(texturedProgram, fontTexture, "Over", 0.2f, -0.1f);

	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.85f, -0.75f, 1.0f));
	texturedProgram.SetModelMatrix(modelMatrix);
	DrawText(texturedProgram, fontTexture, "Press SPACE to return", 0.1f, -0.01f);
}

void drawBackground(ShaderProgram& program, int tileMapTexture) {
	modelMatrix = glm::mat4(1.0f);
	program.SetModelMatrix(modelMatrix);

	std::vector<float> vertexData;
	std::vector<float> texCoordData;

	for (int i = 0; i < BACKGROUND_LAYERS; i++) {
		for (int y = 0; y < mapHeight; y++) {
			for (int x = 0; x < mapWidth; x++) {
				if (backgroundData[i][y][x] != 0) {
					float u = (float)(((int)backgroundData[i][y][x]) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
					float v = (float)(((int)backgroundData[i][y][x]) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;

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
	}

	glBindTexture(GL_TEXTURE_2D, tileMapTexture);

	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program.positionAttribute);

	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program.texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, vertexData.size() / 2);

	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);

	/*
	for (size_t i = 0; i < backgroundTexture.size(); ++i) {
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(-state.player->position.x, -state.player->position.y, -state.player->position.z));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(20.0f, 20.0f, 1.0f));
		texturedProgram.SetModelMatrix(modelMatrix);

		glBindTexture(GL_TEXTURE_2D, backgroundTexture[i]);

		float tileVertices[] = { -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f };

		glVertexAttribPointer(texturedProgram.positionAttribute, 2, GL_FLOAT, false, 0, tileVertices);
		glEnableVertexAttribArray(texturedProgram.positionAttribute);

		float tileTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(texturedProgram.texCoordAttribute, 2, GL_FLOAT, false, 0, tileTexCoords);
		glEnableVertexAttribArray(texturedProgram.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(texturedProgram.positionAttribute);
		glDisableVertexAttribArray(texturedProgram.texCoordAttribute);
	}
	*/
}

void RenderGameLevel(GameState& state) {
	drawBackground(texturedProgram, tileMapTexture);
	drawTileMap(texturedProgram, tileMapTexture);

	if (state.player->isAttacking)
		state.player->Draw(texturedProgram, attackAnimation[currentIndex[4]]);
	else if (fabs(state.player->velocity.x) > 0.3f && state.player->collidedBottom)
		state.player->Draw(texturedProgram, moveAnimation[currentIndex[1]]);
	else if (state.player->velocity.y > 0.0f)
		state.player->Draw(texturedProgram, jumpAnimation[currentIndex[2]]);
	else if (state.player->velocity.y < 0.0f)
		state.player->Draw(texturedProgram, fallAnimation[currentIndex[3]]);
	else
		state.player->Draw(texturedProgram, idleAnimation[currentIndex[0]]);

	// Code to make camera follow player

	//camera = glm::vec3((camera_window.w + camera_window.x) / 2, (camera_window.y + camera_window.z) / 2, 1.0f);
	camera = glm::vec3(-state.player->position.x, -state.player->position.y, -state.player->position.z);
	viewMatrix = glm::mat4(1.0f);
	viewMatrix = glm::translate(viewMatrix, camera);
	projectionMatrix = glm::ortho(-1.777f*4, 1.777f*4, -1.0f*4, 1.0f*4, -1.0f*4, 1.0f*4);
	
	texturedProgram.SetProjectionMatrix(projectionMatrix);
	texturedProgram.SetViewMatrix(viewMatrix);
	
	for (int i = 0; i < state.enemies.size(); ++i) {
		if (state.enemies[i]->health)
			state.enemies[i]->Draw(texturedProgram, idleEnemyAnimation[currentIndex[5]]);
	}

	drawUI(texturedProgram, UITexture);
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

	int gridX = -1;
	int gridY = -1;

	if (state.player->velocity.y < 0)
		worldToTileCoordinates(state.player->position.x, state.player->position.y - state.player->size.y / 2, &gridX, &gridY);
	else if (state.player->velocity.y > 0)
		worldToTileCoordinates(state.player->position.x, state.player->position.y + state.player->size.y / 2, &gridX, &gridY);

	if (gridY > -1 && gridY < mapHeight) {
		if (mapData[gridY][gridX] != 0) {
			if (state.player->velocity.y < 0)
				state.player->collisionY(*tiles[gridY][gridX], (-TILE_SIZE * gridY));
			else if (state.player->velocity.y > 0)
				state.player->collisionY(*tiles[gridY][gridX], (-TILE_SIZE * gridY) - TILE_SIZE);
		}
	}

	if (state.player->velocity.x < 0)
		worldToTileCoordinates(state.player->position.x - state.player->size.x / 2, state.player->position.y, &gridX, &gridY);
	else if (state.player->velocity.x > 0)
		worldToTileCoordinates(state.player->position.x + state.player->size.y / 2, state.player->position.y, &gridX, &gridY);

	if (gridX > -1 && gridX < mapWidth) {
		if (mapData[gridY][gridX] != 0) {
			if (state.player->velocity.x < 0)
				state.player->collisionX(*tiles[gridY][gridX], (TILE_SIZE * gridX) + TILE_SIZE);
			else if (state.player->velocity.x > 0)
				state.player->collisionX(*tiles[gridY][gridX], TILE_SIZE * gridX);
		}
	}

	for (int i = 0; i < state.enemies.size(); ++i) {
		if (state.enemies[i]->health) {
			if (fabs(state.player->position.x - state.enemies[i]->position.x) < 2.0f && fabs(state.player->position.y - state.enemies[i]->position.y) < 2.0f) {
				if (state.player->position.x > state.enemies[i]->position.x)
					state.enemies[i]->acceleration.x = -0.75f;
				else if (state.player->position.x < state.enemies[i]->position.x)
					state.enemies[i]->acceleration.x = 0.75f;
			}
			else if (fabs(state.player->position.x - state.enemies[i]->position.x) > 5.0f && fabs(state.player->position.y - state.enemies[i]->position.y) > 5.0f)
				state.enemies[i]->acceleration.x = 0.0f;

			state.enemies[i]->resetCollisionFlags();
			state.enemies[i]->velocity.x = lerp(state.enemies[i]->velocity.x, 0.0f, elapsed * friction.x);
			state.enemies[i]->velocity.y = lerp(state.enemies[i]->velocity.y, 0.0f, elapsed * friction.y);

			state.enemies[i]->velocity.x += state.enemies[i]->acceleration.x * elapsed;
			state.enemies[i]->velocity.y += state.enemies[i]->acceleration.y * elapsed;

			state.enemies[i]->position.x += state.enemies[i]->velocity.x * elapsed;
			state.enemies[i]->position.y += state.enemies[i]->velocity.y * elapsed;

			int gridX = -1;
			int gridY = -1;
			
			if (state.enemies[i]->velocity.y < 0)
				worldToTileCoordinates(state.enemies[i]->position.x, state.enemies[i]->position.y - state.enemies[i]->size.y / 2, &gridX, &gridY);
			else if (state.enemies[i]->velocity.y > 0)
				worldToTileCoordinates(state.enemies[i]->position.x, state.enemies[i]->position.y + state.enemies[i]->size.y / 2, &gridX, &gridY);

			if (gridY > -1 && gridY < mapHeight) {
				if (mapData[gridY][gridX] != 0) {
					if (state.enemies[i]->velocity.y < 0)
						state.enemies[i]->collisionY(*tiles[gridY][gridX], -TILE_SIZE * gridY);
					else if (state.enemies[i]->velocity.y > 0)
						state.enemies[i]->collisionY(*tiles[gridY][gridX], (-TILE_SIZE * gridY) - TILE_SIZE);
				}
			}

			if (state.enemies[i]->velocity.x < 0)
				worldToTileCoordinates(state.enemies[i]->position.x - state.enemies[i]->size.x / 2, state.enemies[i]->position.y, &gridX, &gridY);
			else if (state.enemies[i]->velocity.x > 0)
				worldToTileCoordinates(state.enemies[i]->position.x + state.enemies[i]->size.y / 2, state.enemies[i]->position.y, &gridX, &gridY);

			if (gridX > -1 && gridX < mapWidth) {
				if (mapData[gridY][gridX] != 0) {
					if (state.enemies[i]->velocity.x < 0)
						state.enemies[i]->collisionX(*tiles[gridY][gridX], (TILE_SIZE * gridX) + TILE_SIZE);
					else if (state.enemies[i]->velocity.x > 0)
						state.enemies[i]->collisionX(*tiles[gridY][gridX], TILE_SIZE * gridX);
				}
			}

			if (state.player->collidesWith(*state.enemies[i])) {
				if (state.player->isAttacking) {
					state.enemies[i]->health = 0;
					state.player->gold += 5;
				}
				else {
					//state.player->health -= 10;
				}
			}
		}
	}

	hungerTimer += elapsed;
	if (state.player->hunger && hungerTimer > 12.0f) {
		state.player->hunger--;
		hungerTimer = 0.0f;
	}
	if (state.player->hunger == 0) {
		starvationTimer += elapsed;
		if (starvationTimer > 3.0f) {
			state.player->health -= 25;
			starvationTimer = 0.0f;
		}
	}

	if (state.player->health <= 0) {
		mode = STATE_GAME_OVER;
		Mix_HaltMusic();
	}

	/*
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
	*/
}

void ProcessMainMenuInput() {
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
			done = true;
		}
		else if (event.type == SDL_KEYDOWN) {
			if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
				mode = STATE_GAME_LEVEL;
				Mix_HaltMusic();
				Mix_PlayMusic(game_music, -1);
				break;
			}
		}
	}
}

void ProcessGameOverInput() {
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
			done = true;
		}
		else if (event.type == SDL_KEYDOWN) {
			if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
				mode = STATE_MAIN_MENU;
				Mix_PlayMusic(menu_music, -1);
				currentLevel = 0;
				resetEntities();
				quitTimer = 0.0f;
				break;
			}
		}
	}
}

void ProcessGameLevelInput(GameState& state) {
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
			done = true;
		}
		else if (event.type == SDL_KEYDOWN) {
			if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
				quitTimer += elapsed;
				if (quitTimer > 3.0f) {
					mode = STATE_MAIN_MENU;
					Mix_HaltMusic();
					Mix_PlayMusic(menu_music, -1);
					currentLevel = 0;
					resetEntities();
					quitTimer = 0.0f;
				}
			}
			if (event.key.keysym.scancode == SDL_SCANCODE_1 && state.player->potions) {
				state.player->health += 50;
				if (state.player->health > 100)
					state.player->health = 100;
				state.player->potions--;
			}
			if (event.key.keysym.scancode == SDL_SCANCODE_2 && state.player->food) {
				state.player->hunger += 50;
				if (state.player->hunger > 100)
					state.player->hunger = 100;
				state.player->food--;
			}
			if (event.key.keysym.scancode == SDL_SCANCODE_DELETE && state.player->hunger) {
				state.player->hunger -= 5;
			}
			if (event.key.keysym.scancode == SDL_SCANCODE_SPACE && state.player->collidedBottom) {
				state.player->velocity.y = 2.0f;
			}
			if (event.key.keysym.scancode == SDL_SCANCODE_DOWN && !state.player->collidedBottom) {
				state.player->acceleration.y = -2.0f;
			}
			else {
				state.player->acceleration.y = -0.981f;
			}
		}
		else if (event.type == SDL_MOUSEBUTTONDOWN) {
			if (event.button.button == SDL_BUTTON_LEFT) {
				int mouseX, mouseY;
				SDL_GetMouseState(&mouseX, &mouseY);
			}
		}
	}

	const Uint8 *keys = SDL_GetKeyboardState(NULL);

	state.player->acceleration.x = 0.0f;

	if (keys[SDL_SCANCODE_UP]) {
		if (changeLevel()) {
			Mix_HaltMusic();
			if (currentLevel)
				Mix_PlayMusic(dungeon_music, -1);
			else 
				Mix_PlayMusic(game_music, -1);
			
			for (int i = 0; i < state.enemies.size(); ++i)
				delete state.enemies[i];
			state.enemies.clear();
			readMapData();

			for (int y = 0; y < mapHeight; y++) {
				for (int x = 0; x < mapWidth; x++) {
					if (mapData[y][x] != 0) {
						Entity* tile = new Entity();
						tile->position = glm::vec3((x * TILE_SIZE), (-y * TILE_SIZE), 0.0f);
						tiles[y][x] = tile;
					}
				}
			}
		}
	}

	if (keys[SDL_SCANCODE_LEFT])
		state.player->acceleration.x = -1.0f;
	else if (keys[SDL_SCANCODE_RIGHT])
		state.player->acceleration.x = 1.0f;
	if (keys[SDL_SCANCODE_Z] && state.player->collidedBottom)
		state.player->isAttacking = true;
	else
		state.player->isAttacking = false;
}

void Render() {
	glClearColor(0.529f, 0.807f, 0.980f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	switch (mode) {
	case STATE_MAIN_MENU:
		RenderMainMenu();
		break;
	case STATE_GAME_LEVEL:
		RenderGameLevel(state);
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
	case STATE_GAME_OVER:
		ProcessGameOverInput();
		break;
	}
}

void Setup() {
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Final Project", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
	glewInit();
#endif

	glViewport(0, 0, 1280, 720);
	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);

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
	spriteTexture = LoadTexture(RESOURCE_FOLDER"assets/adventurer-sheet.png");
	enemyTexture = LoadTexture(RESOURCE_FOLDER"assets/enemy_idle.png");
	UITexture = LoadTexture(RESOURCE_FOLDER"assets/bars.png");

	backgroundTexture.push_back(LoadTexture(RESOURCE_FOLDER"assets/backgrounds/forest/Layer_0010_1.png"));
	backgroundTexture.push_back(LoadTexture(RESOURCE_FOLDER"assets/backgrounds/forest/Layer_0009_2.png"));
	backgroundTexture.push_back(LoadTexture(RESOURCE_FOLDER"assets/backgrounds/forest/Layer_0008_3.png"));
	backgroundTexture.push_back(LoadTexture(RESOURCE_FOLDER"assets/backgrounds/forest/Layer_0007_Lights.png"));
	backgroundTexture.push_back(LoadTexture(RESOURCE_FOLDER"assets/backgrounds/forest/Layer_0006_4.png"));
	backgroundTexture.push_back(LoadTexture(RESOURCE_FOLDER"assets/backgrounds/forest/Layer_0005_5.png"));
	backgroundTexture.push_back(LoadTexture(RESOURCE_FOLDER"assets/backgrounds/forest/Layer_0004_Lights.png"));
	backgroundTexture.push_back(LoadTexture(RESOURCE_FOLDER"assets/backgrounds/forest/Layer_0003_6.png"));
	backgroundTexture.push_back(LoadTexture(RESOURCE_FOLDER"assets/backgrounds/forest/Layer_0002_7.png"));
	backgroundTexture.push_back(LoadTexture(RESOURCE_FOLDER"assets/backgrounds/forest/Layer_0001_8.png"));
	backgroundTexture.push_back(LoadTexture(RESOURCE_FOLDER"assets/backgrounds/forest/Layer_0000_9.png"));
	
	playerSprite = SheetSprite(spriteTexture, 7, 11);
	enemySprite = SheetSprite(enemyTexture, 3, 4);
	tileMapTexture = LoadTexture(RESOURCE_FOLDER"assets/fantasy.png");
	
	state.player = new Entity(ENTITY_PLAYER, false, playerSprite);
	state.player->size = glm::vec3(2.0f, 2.0f, 1.0f);
	state.player->health = 100;
	state.player->hunger = 100;
	state.player->gold = 20;
	state.player->potions = 5;
	state.player->food = 5;
	readMapData();

	for (int y = 0; y < mapHeight; y++) {
		for (int x = 0; x < mapWidth; x++) {
			if (mapData[y][x] != 0) {
				Entity* tile = new Entity();
				tile->position = glm::vec3((x * TILE_SIZE), (-y * TILE_SIZE), 0.0f);
				tiles[y][x] = tile;
			}
		}
	}

	menu_music = Mix_LoadMUS(RESOURCE_FOLDER"assets/audio/adventure.mp3");
	game_music = Mix_LoadMUS(RESOURCE_FOLDER"assets/audio/mother_nature.mp3");
	dungeon_music = Mix_LoadMUS(RESOURCE_FOLDER"assets/audio/ambient.mp3");
	footstep1 = Mix_LoadWAV(RESOURCE_FOLDER"assets/audio/sound_effects/footstep08.ogg");
	footstep2 = Mix_LoadWAV(RESOURCE_FOLDER"assets/audio/sound_effects/footstep09.ogg");

	Mix_VolumeMusic(6);
	Mix_PlayMusic(menu_music, -1);
	
	//resetEntities();
}

int main(int argc, char *argv[])
{
	Setup();
	//Mix_VolumeChunk(footstep2, 2);
	//Mix_PlayChannel(-1, footstep1, -1);

  	while (!done) {
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		elapsed += accumulator;
		animationElapsed += elapsed;

		if (animationElapsed > 1.0f / framesPerSecond[0]) {
			for (int i = 0; i < 6; ++i) {
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

	Mix_FreeMusic(menu_music);
	Mix_FreeMusic(game_music);
	Mix_FreeMusic(dungeon_music);
	Mix_FreeChunk(footstep1);
	Mix_FreeChunk(footstep2);

	SDL_Quit();
	return 0;
}