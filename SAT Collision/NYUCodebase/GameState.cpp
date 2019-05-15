#include "GameState.h"

GameState::GameState() {};

GameState::~GameState() {
	delete player;

	for (size_t i = 0; i < enemies.size(); ++i)
		delete enemies[i];
};