#include "ants.h"

const int NUM_AGENTS = 4;
const int MAX = 255;
const char FOOD = '*';
const char WATER = '%';
const char LAND = '.';
const char DEAD = '!';
const char ENEMY_HILL = 'H';

const int FOOD_GOAL = 0;
const int HILL_GOAL = 1;
const int EXPLORE_GOAL = 2;

// clear the diffusion agents at the given tile
void clearDiffusion(struct tile *tile) {
    int i;
    for (i = 0; i < NUM_AGENTS; ++i)
        tile->agents[i] = 0;
}

// precalculate tiles around an ant to set as visible
//should only be called once, since the vision offsets don't change
void _init_vision_offsets(struct game_info *game_info) {
    int mx = (int)(sqrt(game_info->viewradius_sq));
    fprintf(stderr, "mx = %d\n", mx);
    fprintf(stderr, "rows = %dcols = %d\n", game_info->rows, game_info->cols);    
    int count = 0;
    int d_row, d_col;
    // find the number of offsets given the viewradius
    // for memory allocation
    for (d_row = -mx; d_row < mx+1; ++d_row)
        for (d_col = -mx; d_col < mx+1; ++d_col)
            if (d_row*d_row + d_col*d_col <= game_info->viewradius_sq)
                count++;                                              

    game_info->vision_offset_length = count;
    // first dimension is index offsets, second is row/col
    game_info->vision_offsets_sq = malloc(count*sizeof(int*));
    int i;
    for (i = 0; i < count;  ++i)
        game_info->vision_offsets_sq[i] = malloc(2*sizeof(int));
    
    count = 0;
    for (d_row = -mx; d_row < mx+1; ++d_row) {
        for (d_col = -mx; d_col < mx+1; ++d_col) {
            int d = d_row*d_row + d_col*d_col;
            if (d <= game_info->viewradius_sq) {
                // Create all negative offsets so vision will
                // wrap around the edges properly
                int row_offset = (d_row % game_info->rows);
                int col_offset = (d_col % game_info->cols);
                if (row_offset >= 0) row_offset -= game_info->rows;
                if (col_offset >= 0) col_offset -= game_info->cols;      
                game_info->vision_offsets_sq[count][0] = row_offset;
                game_info->vision_offsets_sq[count][1] = col_offset;
                count++;
            }
        }
    }    
}

// initializes the game_info structure on the very first turn
// function is not called after the game has started

void _init_ants(char *data, struct game_info *game_info) {
    char *replace_data = data;

    while (*replace_data != '\0') {
        if (*replace_data == '\n')
            *replace_data = '\0';
        ++replace_data;
    }

    while (42) {
        char *value = data;

        while (*++value != ' ');
        ++value;

        int num_value = atoi(value);

        switch (*data) {
            case 'l':
                game_info->loadtime = num_value;
                break;

            case 't':
                if (*(data + 4) == 't')
                    game_info->turntime = num_value;
                else
                    game_info->turns = num_value;
                break;

            case 'r':
                game_info->rows = num_value;
                break;

            case 'c':
                game_info->cols = num_value;
                break;

            case 'v':
                game_info->viewradius_sq = num_value;
                break;

            case 'a':
                game_info->attackradius_sq = num_value;
                break;

            case 's':
                if (*(data + 1) == 'p')
                    game_info->spawnradius_sq = num_value;
                else
                    game_info->seed = num_value;
                break;

        }

        data = value;

        while (*++data != '\0');
        ++data;

        if (strcmp(data, "ready") == 0)
            break;
    }
    _init_vision_offsets(game_info);
}

// updates game data with locations of ants and food
// only the ids of your ants are preserved
void _init_game(struct game_info *game_info, struct game_state *game_state) {
    game_state->curr_turn++;
    int map_len = game_info->rows*game_info->cols;
    int my_count = 0;
    int enemy_count = 0;
    int food_count = 0;
    int dead_count = 0;
    int hill_count = 0;
    int i, j;

    for (i = 0; i < map_len; ++i) {
        char current = game_info->map[i].state;

        if (current == '?' || current == LAND || current == WATER)
            continue;
        else if (current == FOOD)
            ++food_count;
        else if (current == 'a')
            ++my_count;
        else if (current == 'A') {
            ++my_count;
            ++hill_count;
        }
        else if (isdigit(current))
            ++hill_count;
        else if (current & 0x80)
            ++dead_count;
        else if (isupper(current)){
            ++hill_count;
            ++enemy_count;
        }
        else
            ++enemy_count;
    }

    struct my_ant *my_old = 0;
    int my_old_count = game_state->my_count;

    game_state->my_count = my_count;
    game_state->enemy_count = enemy_count;
    game_state->food_count = food_count;
    game_state->dead_count = dead_count;
    game_state->hill_count = hill_count;

    if (game_state->my_ants != 0)
        my_old = game_state->my_ants;

    if (game_state->enemy_ants != 0)
        free(game_state->enemy_ants);

    if (game_state->food != 0)
        free(game_state->food);

    if (game_state->dead_ants != 0)
        free(game_state->dead_ants);

    if (game_state->hill != 0)
        free(game_state->hill);

    game_state->my_ants = malloc(my_count*sizeof(struct my_ant));

    if (enemy_count > 0)
        game_state->enemy_ants = malloc(enemy_count*sizeof(struct basic_ant));
    else
        game_state->enemy_ants = 0;

    if (dead_count > 0)
        game_state->dead_ants = malloc(dead_count*sizeof(struct basic_ant));
    else
        game_state->dead_ants = 0;

    if (hill_count > 0)
        game_state->hill = malloc(hill_count*sizeof(struct hill));
    else
        game_state->hill = 0;

    if (food_count > 0)
        game_state->food = malloc(food_count*sizeof(struct food));
    else
        game_state->food = 0;

    int my_count_start = my_count;

    for (i = 0; i < game_info->rows; ++i) {
        for (j = 0; j < game_info->cols; ++j) {
            char current = game_info->map[game_info->cols*i + j].state;
            if (current == '?' || current == LAND || current == WATER)
                continue;

            if (current == FOOD) {
                --food_count;

                game_state->food[food_count].row = i;
                game_state->food[food_count].col = j;
            } else if (isdigit(current)) {
                --hill_count;

                game_state->hill[hill_count].row = i;
                game_state->hill[hill_count].col = j;
                game_state->hill[hill_count].player = current - '0';
            } else if (current == 'a' || current == 'A') {
                if (isupper(current)) {
                    --hill_count;

                    game_state->hill[hill_count].row = i;
                    game_state->hill[hill_count].col = j;
                    game_state->hill[hill_count].player = current - 'A';
                }
                --my_count;

                int keep_id = -1;
                int k = 0;

                if (my_old != 0) {
                    for (; k < my_old_count; ++k) {
                        if (my_old[k].row == i && my_old[k].col == j) {
                            keep_id = my_old[k].id;
                            break;
                        }
                    }
                }

                game_state->my_ants[my_count].row = i;
                game_state->my_ants[my_count].col = j;

                if (keep_id == -1)
                    game_state->my_ants[my_count].id = ++game_state->my_ant_index;
                else
                    game_state->my_ants[my_count].id = keep_id;
            } else if (current & 0x80) {
                --dead_count;

                game_state->dead_ants[dead_count].row = i;
                game_state->dead_ants[dead_count].col = j;
                game_state->dead_ants[dead_count].player = (current & (~0x80));
                
                game_info->map[game_info->cols*i + j].state = DEAD;
            } else if (isupper(current)) {
                --hill_count;

                game_state->hill[hill_count].row = i;
                game_state->hill[hill_count].col = j;
                game_state->hill[hill_count].player = current - 'A';
                --enemy_count;

                game_info->map[i*game_info->cols + j].state = ENEMY_HILL;
                game_state->enemy_ants[enemy_count].row = i;
                game_state->enemy_ants[enemy_count].col = j;
                game_state->enemy_ants[enemy_count].player = current - 'A';

            } else {
                --enemy_count;

                game_state->enemy_ants[enemy_count].row = i;
                game_state->enemy_ants[enemy_count].col = j;
                game_state->enemy_ants[enemy_count].player = current - 'a';
            } 
        }
    }

    if (my_old != 0)
        free(my_old);
}

// Updates the map.
//
//    %   = Walls       (the official spec calls this water,
//                      in either case it's simply space that is occupied)
//    .   = Land        (territory that you can walk on)
//    a   = Your Ant
// [b..z] = Enemy Ants
//    !   = Dead Ants   (disappear after one turn)
//    *   = Food
// [A..Z] = hill + ant
// [0..9] = hill
//    ?   = Unknown     (not used in latest engine version, unknowns are assumed to be land)


void _init_map(char *data, struct game_info *game_info) {

    int map_len = game_info->rows*game_info->cols;
    int i;
    
    if (game_info->map == 0) {
        game_info->map = malloc(game_info->rows*game_info->cols*sizeof(struct tile));
        for (i = 0; i < map_len; ++i) {
            struct tile *newTile = malloc(sizeof(struct tile));
            newTile->state = LAND;            
            newTile->row = i/game_info->cols;
            newTile->col = i%game_info->cols;
            newTile->lastSeen = 0;
            newTile->seen = 0;
            newTile->visible = 0;
            clearDiffusion(newTile);
            game_info->map[i] = *newTile;
        }       
    }

    for (i = 0; i < map_len; ++i)
        if (game_info->map[i].state != WATER)
            game_info->map[i].state = LAND;

    while (*data != 0) {
        char *tmp_data = data;
        int arg = 0;

        while (*tmp_data != '\n') {
            if (*tmp_data == ' ') {
                *tmp_data = '\0';
                ++arg;
            }

            ++tmp_data;
        }

        char *tmp_ptr = tmp_data;
        tmp_data = data;

        tmp_data += 2;
        int jump = strlen(tmp_data) + 1;

        int row = atoi(tmp_data);
        int col = atoi(tmp_data + jump);
        char var3 = -1;

        if (arg > 2) {
            jump += strlen(tmp_data + jump) + 1;
            var3 = *(tmp_data + jump);
        }

        int offset = row*game_info->cols + col;

        switch (*data) {

            case 'w':
                game_info->map[offset].state = WATER;
                break;
            case 'a':
                if (isdigit(game_info->map[offset].state))
                    game_info->map[offset].state = var3 - '0' + 'A';
                else
                   game_info->map[offset].state = var3 - '0' + 'a';
                break;
            case 'd':
                game_info->map[offset].state = (((unsigned char) (var3 - '0')) + 0x80);
                break;
            case 'f':
                game_info->map[offset].state = FOOD;
                break;
            case 'h':
                game_info->map[offset].state = var3;
                break;
            default: 
                break;

        }
        data = tmp_ptr + 1;
    }
}

// set all spaces as not visible,
// loop through ants and set all tiles around ant as visible
void updateVision(struct game_info *Info, struct game_state *Game) {
    int ant_length = sizeof(Game->my_ants)/sizeof(struct my_ant);
    int i, j;

    for (i = 0; i < Info->rows; ++i)
        for (j = 0; j < Info->cols; ++j)
            Info->map[i*Info->cols + j].visible = 0;                
    
    for (i = 0; i < ant_length; ++i) {
        struct my_ant ant = Game->my_ants[i];
        for (j = 0; j < Info->vision_offset_length; ++j) {
            int v_row = Info->vision_offsets_sq[j][0] + ant.row;
            int v_col = Info->vision_offsets_sq[j][1] + ant.col;
            Info->map[v_row*Info->cols + v_col].visible = 1;
        }
    }
}

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

void diffusion(struct tile *tile, struct game_info *Info, struct game_state *Game) {
    int goalsToDiffuse[] = {0, 0, 0, 0};
    
    if (tile->state == WATER) {
        clearDiffusion(tile);
        return;
    }
    if (tile->state == FOOD)
        tile->agents[FOOD_GOAL] = MAX;
    else
        goalsToDiffuse[FOOD_GOAL] = 1;
            
    if (tile->state == ENEMY_HILL)
        tile->agents[HILL_GOAL] = MAX;
    else
        goalsToDiffuse[HILL_GOAL] = 1;

    if (tile->visible) {
        tile->lastSeen = Game->curr_turn;
        if (!tile->seen)
            tile->seen = 1;
    }
    
    if (tile->seen)
        goalsToDiffuse[EXPLORE_GOAL] = 1;
    else
        tile->agents[EXPLORE_GOAL] = MAX;

    int goal;
    for (goal = 0; goal < NUM_AGENTS; ++goal) {
        if (goalsToDiffuse[goal]) {
            char up = tileInDirection('N', tile, Info, Game)->agents[goal];
            char down = tileInDirection('S', tile, Info, Game)->agents[goal];
            char left = tileInDirection('W', tile, Info, Game)->agents[goal];
            char right = tileInDirection('E', tile, Info, Game)->agents[goal];
            tile->agents[goal] = 0.22*(up +down + left + right);
        }
    }
}

int diffusionValueAt(int goal, struct my_ant *ant, struct game_info *Info) {
    return Info->map[ant->row*Info->cols + ant->col].agents[goal];
}
