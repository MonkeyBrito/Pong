#pragma once

#include "../defines.h"
#include "../logger.h"
#include "../renderer/shared_render_types.h"

#define MAX_ENTITIES 100

typedef struct Entity {
    Transform transform;
} Entity;

typedef struct GameState {
    uint32_t entity_count;
    Entity entities[MAX_ENTITIES];
} GameState;

bool init_game(GameState *game_state);

void update_game(GameState *game_state);