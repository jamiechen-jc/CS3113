#ifndef GameState_h
#define GameState_h

#include "Entity.h"

struct GameState {
	GameState();

	Entity player;
	Entity enemies[12];
	Entity bullets[30];
};

#endif