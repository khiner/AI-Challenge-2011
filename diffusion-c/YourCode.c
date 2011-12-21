#include "ants.h"

struct tile *tileInDirection(char direction, struct tile *tile,
                      struct game_info *Info, struct game_state *Game) {
    
    // defining things just so we can do less writing
    // UP and DOWN move up and down rows while LEFT and RIGHT
    // move side to side. The map is just one big array.

    #define UP -Info->cols
    #define DOWN Info->cols
    #define LEFT -1
    #define RIGHT 1
    
    // the location within the map array where our ant is currently

    int offset = tile->row*Info->cols + tile->col;

    switch(direction) {
    case 'W':
        if (tile->col != 0)
            return &Info->map[offset + LEFT];
        else
            return &Info->map[offset + Info->cols - 1];
        break;
    case 'E':
        if (tile->col != Info->cols - 1)
            return &Info->map[offset + RIGHT];
        else
            return &Info->map[offset - Info->cols + 1];
        break;
    case 'N':
        if (tile->row != 0)
            return &Info->map[offset + UP];
        else
            return &Info->map[offset + (Info->rows - 1)*Info->cols];
        break;
    case 'S':
        if (tile->row != Info->rows - 1)
            return &Info->map[offset + DOWN];
        else
            return &Info->map[offset - (Info->rows - 1)*Info->cols];
        break;
    default:
        return '\0';
    }
}

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

    int goal;
    for (goal = 0; goal < NUM_AGENTS; ++goal) {
        if (goalsToDiffuse[goal]) {
            float up = tileInDirection('N', tile, Info, Game)->agents[goal];
            float down = tileInDirection('S', tile, Info, Game)->agents[goal];
            float left = tileInDirection('W', tile, Info, Game)->agents[goal];
            float right = tileInDirection('E', tile, Info, Game)->agents[goal];
            tile->agents[goal] = 0.20*(up +down + left + right);
        }
    }
}

void diffuseAll(struct game_info *Info, struct game_state *Game) {
    int i, j;
    for (i = 0; i < Info->rows; ++i)
        for (j = 0; j < Info->cols; ++j)
            diffuse(&Info->map[i*Info->cols + j], Info, Game);
}

// sanity check the move of ant in direction,
// send the move to the tournament engine, and
// update ant location to avoid collisions
int do_move_direction(struct tile *ant, char dir,
                       struct game_info *Info, struct game_state *Game) {
    struct tile *newTile = tileInDirection(dir, ant, Info, Game);
    if (newTile->state != WATER && newTile->state != MY_ANT &&
        newTile->state != MY_HILL && newTile->state != MY_ANT_AND_HILL &&
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
    char nextBestDirection = 0;
    char directions[4] = {'S', 'E', 'W', 'N'};
    int i;
    for (i = 0; i < 4; ++i) {
        char direction = directions[i];
        struct tile *otherTile = tileInDirection(direction, tile, Info, Game);
        float val = otherTile->agents[goal];
        if (val > highestVal) {
            highestVal = val;
            nextBestDirection = bestDirection;
            bestDirection = direction;
        }
    }
    // if we're blocked at the best direction, do the next best thing
    if (!(bestDirection && do_move_direction(tile, bestDirection, Info, Game)))
        if (nextBestDirection)
            do_move_direction(tile, nextBestDirection, Info, Game);
}

void do_turn(struct game_state *Game, struct game_info *Info) {
    // move
    int i;    
    for (i = 0; i < Game->my_count; ++i) {

        struct tile *ant = &Game->my_ants[i];
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

