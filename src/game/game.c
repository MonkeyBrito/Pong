#include "game.h"

internal Entity *create_entity(GameState *game_state, Transform transform) {
    Entity *e = NULL;
    if (game_state->entity_count < MAX_ENTITIES) {
        e = &game_state->entities[game_state->entity_count++];
        e->transform = transform;
    } else {
        CAKEZ_ASSERT(0, "Reached Maximum amount of Entities!");
    }
    return e;
}

bool init_game(GameState *game_state) {
    for (uint32_t i = 0; i < 10; i++) {
        for (uint32_t j = 0; j < 10; j++) {
            Entity *e = create_entity(game_state, (Transform){i * 120.0f, j * 60.0f, 70.0f, 70.0f});
        }
    }

    return true;
}

void update_game(GameState *game_state) {
    // Does nothing
    for (uint32_t i = 0; i < game_state->entity_count; i++) {
        Entity *e = &game_state->entities[i];
        // This is frame rate dependent
        e->transform.x_pos += 0.01f;
    }
}