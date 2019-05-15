#ifndef GameState_h
#define GameState_h

#include "Entity.h"
#include <vector>

struct GameState {
	GameState();
	~GameState();

	Entity* player;
	std::vector<Entity*> enemies;
};

#endif