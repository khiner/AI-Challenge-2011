#include "ants.h"

void diffuse(struct tile *tile, struct game_info *Info, struct game_state *Game) {
    int goalsToDiffuse[] = {0, 0, 0};
    
    if (tile->state == WATER) {
        clearDiffusion(tile);
        return;
    }
    if (tile->state == FOOD)
        tile->agents[FOOD_GOAL] = 1.0;
    else
        goalsToDiffuse[FOOD_GOAL] = 1;
            
    if (tile->state == ENEMY_HILL || tile->state == ENEMY_ANT_AND_HILL)
        tile->agents[HILL_GOAL] = 1.0;
    else
        goalsToDiffuse[HILL_GOAL] = 1;

    if (tile->visible) {
        tile->lastSeen = Info->curr_turn;
        if (!tile->seen)
            tile->seen = 1;
    }

    if (tile->seen)
        goalsToDiffuse[EXPLORE_GOAL] = 1;
    else
        tile->agents[EXPLORE_GOAL] = 1.0;

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

void outputMove(struct tile *tile, int goal, struct game_info *Info, struct game_state *Game) {
    float highestVal = 0.0;
    char bestDirection = 0;
    int i, j;
    for (i = 0; i < 4; ++i) {
        char direction = directions[i];
        struct tile *otherTile = tileInDirection(direction, tile, Info, Game);
        float val = otherTile->agents[goal];
        if (val > highestVal && isLegal(otherTile)) {
            highestVal = val;
            bestDirection = direction;
        }
    }
    if (bestDirection)
        do_move_direction(tile, bestDirection, Info, Game);
}

void do_turn(struct game_state *Game, struct game_info *Info) {
    // move
    int i;    
    for (i = 0; i < Game->my_count; ++i) {
        struct tile *ant = &Info->map[Game->my_ants[i]];
        float hill = ant->agents[HILL_GOAL];
        float food = ant->agents[FOOD_GOAL];
        float explore = ant->agents[EXPLORE_GOAL];

        int goal;
        if (hill != 0)
            goal = HILL_GOAL;
        else if (food != 0)
            goal = FOOD_GOAL;
        else
            goal = EXPLORE_GOAL;
            
        outputMove(ant, goal, Info, Game);
    }
}

