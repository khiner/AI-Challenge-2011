#include "ants.h"

void diffuse(struct tile *tile, struct game_info *Info, struct game_state *Game) {
    int goalsToDiffuse[] = {false, false, false, false};
    
    if (tile->state == WATER) {
        clearDiffusion(tile);
        return;
    }
    if (tile->state == FOOD)
        tile->agents[FOOD_GOAL] = 1.0;
    else
        goalsToDiffuse[FOOD_GOAL] = true;
            
    if (tile->state == ENEMY_HILL || tile->state == ENEMY_ANT_AND_HILL)
        tile->agents[HILL_GOAL] = 1.0;
    else
        goalsToDiffuse[HILL_GOAL] = true;

    if (tile->visible) {
        tile->lastSeen = Info->curr_turn;
        if (!tile->seen)
            tile->seen = 1;
    }

    if (tile->seen)
        goalsToDiffuse[EXPLORE_GOAL] = true;
    else
        tile->agents[EXPLORE_GOAL] = 1.0;

    if (tile->state == ENEMY_ANT)
        tile->agents[ENEMY_ANT_GOAL] = 1.0;
    else
        goalsToDiffuse[ENEMY_ANT_GOAL] = true;
    
    int goal, d;
    for (goal = 0; goal < NUM_AGENTS; ++goal) {
        if (goalsToDiffuse[goal]) {
            float neighbor_total = 0.0;
            for (d = 0; d < 4; ++d)
                neighbor_total += tileInDirection(directions[d], tile, Info, Game)->agents[goal];
            
            tile->agents[goal] = 0.20*neighbor_total;
        }
    }
}

void diffuseAll(struct game_info *Info, struct game_state *Game) {
    int i, j;
    for (i = 0; i < Info->rows; ++i)
        for (j = 0; j < Info->cols; ++j)
            diffuse(&Info->map[i*Info->cols + j], Info, Game);
}

bool isLegal(struct tile *tile) {
    return (tile->state != WATER && tile->state != MY_ANT &&
            tile->state != MY_HILL && tile->state != MY_ANT_AND_HILL &&
            tile->combat == SAFE);
}

// sanity check the move of ant in direction,
// send the move to the tournament engine, and
// update ant location to avoid collisions
int do_move_direction(struct tile *ant, char dir,
                       struct game_info *Info, struct game_state *Game) {
    struct tile *newTile = tileInDirection(dir, ant, Info, Game);
    if (isLegal(newTile) &&
        (ant->state == MY_ANT || ant->state == MY_ANT_AND_HILL)) {
        fprintf(stdout, "O %i %i %c\n", ant->row, ant->col, dir);
        // take care of
        ant->state = LAND;
        newTile->state = MY_ANT;
        return 1;
    } else
        return 0;
}

void do_turn(struct game_state *Game, struct game_info *Info) {
    // move
    int i, j;    
    for (i = 0; i < Game->my_count; ++i) {
        struct tile *ant = &Info->map[Game->my_ants[i]];        
        float highestVal = 0.0;
        char bestDirection = 0;
        
        for (j = 0; j < 5; ++j) {            
            char direction = directions[j];
            struct tile *tile = tileInDirection(direction, ant, Info, Game);
            float hill = tile->agents[HILL_GOAL];
            float food = tile->agents[FOOD_GOAL];
            float explore = tile->agents[EXPLORE_GOAL];
            float enemy = tile->agents[ENEMY_ANT_GOAL];
            
            float val = food + hill + explore + enemy;
            if (val > highestVal && isLegal(tile)) {
                highestVal = val;
                bestDirection = direction;
            }
        }

        if (bestDirection)
            do_move_direction(ant, bestDirection, Info, Game);
    }
}

