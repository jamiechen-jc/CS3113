#ifndef GameState_h
#define GameState_h

#include "Entity.h"

struct GameState {
	GameState();

	Entity player;
	Entity enemies[18];
	Entity bullets[10];
};

#endif