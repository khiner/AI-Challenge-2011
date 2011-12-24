#include "ants.h"

const char directions[4] = {'N', 'E', 'S', 'W'};

const int NUM_AGENTS = 3;
const int NUM_DIFFUSIONS = 100;

const char FOOD = '*';
const char WATER = '%';
const char LAND = '.';
const char DEAD = '!';
const int MY_HILL = '0';
const int ENEMY_HILL = '1';
const char MY_ANT = 'a';
const char ENEMY_ANT = 'b';
const char MY_ANT_AND_HILL = 'A';
const char ENEMY_ANT_AND_HILL = 'B';

const int FOOD_GOAL = 0;
const int HILL_GOAL = 1;
const int EXPLORE_GOAL = 2;

const int SAFE = 0;
const int KILL = 1;
const int DIE = 2;

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

// clear the diffusion agents at the given tile
void clearDiffusion(struct tile *tile) {
    int i;
    for (i = 0; i < NUM_AGENTS; ++i)
        tile->agents[i] = 0.0;
}

// precalculate tiles around an ant to set as visible
//should only be called once, since the vision offsets don't change
void _init_vision_offsets(struct game_info *game_info) {
    int mx = (int)(sqrt(game_info->viewradius_sq));
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

void _init_attack_offsets(struct game_info *Info) {
    int mx = (int)(sqrt(Info->attackradius_sq));
    int count = 0;
    int d_row, d_col;
    // find the number of offsets given the viewradius
    // for memory allocation
    for (d_row = -mx; d_row < mx+1; ++d_row)
        for (d_col = -mx; d_col < mx+1; ++d_col)
            if (d_row*d_row + d_col*d_col <= Info->attackradius_sq)
                count++;                                              

    Info->attack_offset_length = count;
    // first dimension is index offsets, second is row/col
    Info->attack_offsets_sq = malloc(count*sizeof(int*));
    int i;
    for (i = 0; i < count;  ++i)
        Info->attack_offsets_sq[i] = malloc(2*sizeof(int));
    
    count = 0;
    for (d_row = -mx; d_row < mx+1; ++d_row) {
        for (d_col = -mx; d_col < mx+1; ++d_col) {
            int d = d_row*d_row + d_col*d_col;
            if (d <= Info->attackradius_sq) {
                // Create all negative offsets so vision will
                // wrap around the edges properly
                int row_offset = (d_row % Info->rows);
                int col_offset = (d_col % Info->cols);
                Info->attack_offsets_sq[count][0] = row_offset;
                Info->attack_offsets_sq[count][1] = col_offset;
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
    game_info->curr_turn = 0;
    _init_vision_offsets(game_info);
    _init_attack_offsets(game_info);    
}

// updates ant and hill lists
void _init_game(struct game_info *game_info, struct game_state *game_state) {
    if (game_state->my_ants != 0)
        free(game_state->my_ants);
    
    if (game_state->enemy_ants != 0)
        free(game_state->enemy_ants);

    if (game_state->my_hills != 0)
        free(game_state->my_hills);

    if (game_state->enemy_hills != 0)
        free(game_state->enemy_hills);
    
    game_state->my_ants = malloc(game_state->my_count*sizeof(int));

    if (game_state->enemy_count > 0)
        game_state->enemy_ants = malloc(game_state->enemy_count*sizeof(int));
    else
        game_state->enemy_ants = 0;

    if (game_state->my_hill_count > 0)
        game_state->my_hills = malloc(game_state->my_hill_count*sizeof(int));
    else
        game_state->my_hills = 0;

    if (game_state->enemy_hill_count > 0)
        game_state->enemy_hills = malloc(game_state->enemy_hill_count*sizeof(int));
    else
        game_state->enemy_hills = 0;
    
    int my_hill_count = 0;
    int enemy_hill_count = 0;
    int my_count = 0;
    int enemy_count = 0;
    int i;
    
    for (i = 0; i < game_info->rows*game_info->cols; ++i) {
        struct tile current = game_info->map[i];
        if (current.state == LAND || current.state == WATER)
                continue;
        else if (current.state == MY_HILL) {
            game_state->my_hills[my_hill_count] = i;
            ++my_hill_count;
        } else if (current.state == ENEMY_HILL) {
            game_state->enemy_hills[enemy_hill_count] = i;
            ++enemy_hill_count;
        } else if (current.state == MY_ANT_AND_HILL) {
            game_state->my_hills[my_hill_count] = i;
            game_state->my_ants[my_count] = i;
            ++my_hill_count;            
            ++my_count;
        } else if (current.state == ENEMY_ANT_AND_HILL) {
            game_state->enemy_hills[enemy_hill_count] = i;
            game_state->enemy_ants[enemy_count] = i;
            ++enemy_hill_count;
            ++enemy_count;
        } else if (current.state == ENEMY_ANT) {
            game_state->enemy_ants[enemy_count] = i;
            ++enemy_count;
        } else if (current.state == MY_ANT) {
            game_state->my_ants[my_count] = i;
            ++my_count;
        }
    }
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


void _init_map(char *data, struct game_info *game_info, struct game_state *game_state) {
    int map_len = game_info->rows*game_info->cols;
    int i;
    int my_count = 0;
    int enemy_count = 0;
    int my_hill_count = 0;
    int enemy_hill_count = 0;
    
    if (game_info->map == 0) {
        game_info->map = malloc(game_info->rows*game_info->cols*sizeof(struct tile));
        for (i = 0; i < map_len; ++i) {
            struct tile *newTile = malloc(sizeof(struct tile));
            newTile->state = LAND;            
            newTile->row = i/game_info->cols;
            newTile->col = i%game_info->cols;
            newTile->lastSeen = 0;
            newTile->my_attack_influence = 0;
            newTile->enemy_attack_influence = 0;   
            newTile->seen = false;
            newTile->visible = false;
            newTile->combat = SAFE;
            clearDiffusion(newTile);
            game_info->map[i] = *newTile;
        }       
    }

    for (i = 0; i < map_len; ++i) {
        struct tile tile = game_info->map[i];
        // keep hills in memory even after they are not visible
        if (tile.state == MY_HILL) ++my_hill_count;
        else if (tile.state == ENEMY_HILL) ++enemy_hill_count;
        else if (tile.state == MY_ANT_AND_HILL) {
            tile.state = MY_HILL;
            ++my_hill_count;
        } else if (tile.state == ENEMY_ANT_AND_HILL) {
            tile.state = ENEMY_HILL;
            ++enemy_hill_count;
        } else if (tile.state != WATER)
            game_info->map[i].state = LAND;
    }
    
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
        
        struct tile *tile = &game_info->map[row*game_info->cols + col];
        
        switch (*data) {
        case 'w':
            tile->state = WATER;
            break;
        case 'a':            
            if (var3 != '0') {
                if (tile->state == ENEMY_HILL)
                    // we've already incremented the enemy_hill_count
                    tile->state = ENEMY_ANT_AND_HILL;
                else
                    tile->state = ENEMY_ANT;
                ++enemy_count;
            } else {
                if (tile->state == MY_HILL)
                    // we've already incremented my_hill_count
                    tile->state = MY_ANT_AND_HILL;
                else
                    tile->state = MY_ANT;
                ++my_count;
            }
            break;
        case 'd':
            if (var3 == '0') // only marking dead for my ants.
                             // otherwise, treat it as LAND
                tile->state = DEAD;
            break;
        case 'f':
            tile->state = FOOD;
            break;
        case 'h':
            if (var3 == '0' && tile->state != MY_HILL) {
                tile->state = MY_HILL;
                ++my_hill_count;
            } else if (var3 != '0' && tile->state != ENEMY_HILL) {
                tile->state = ENEMY_HILL;
                ++enemy_hill_count;
            }
            break;
        default: 
            break;
        }
        data = tmp_ptr + 1;
    }
    
    game_state->my_count = my_count;
    game_state->enemy_count = enemy_count;
    game_state->my_hill_count = my_hill_count;
    game_state->enemy_hill_count = enemy_hill_count;
}

// set all spaces as not visible,
// loop through ants and set all tiles around ant as visible
void updateVision(struct game_info *Info, struct game_state *Game) {
    int i, j;
    // set all tiles as not visible
    for (i = 0; i < Info->rows*Info->cols; ++i)
        Info->map[i].visible = 0;
    for (i = 0; i < Game->my_count; ++i) {
        struct tile ant = Info->map[Game->my_ants[i]];
        for (j = 0; j < Info->vision_offset_length; ++j) {
            int row = (Info->vision_offsets_sq[j][0] + ant.row) % Info->rows;
            int col = (Info->vision_offsets_sq[j][1] + ant.col) % Info->cols;
            if (row < 0) row = Info->rows + row;
            if (col < 0) col = Info->cols + col;
            Info->map[row*Info->cols + col].visible = 1;
        }
    }
}

void updateCombat(struct game_info *Info, struct game_state *Game) {

    int i, j, k;
    // set all tiles as safe
    for (i = 0; i < Info->rows*Info->cols; ++i)
        Info->map[i].combat = SAFE;

    // if there are no visible enemy ants, all tiles are safe
    if (!Game->enemy_count)
        return;

    // clear the attack influence counts
    int num_attack_tiles = 0;    
    for (i = 0; i < Info->rows*Info->cols; ++i) {
        Info->map[i].my_attack_influence = 0;
        Info->map[i].enemy_attack_influence = 0;        
    }
    
    // count how many times my ants could affect each tile after moving one tile
    for (i = 0; i < Game->my_count; ++i) {
        struct tile my_ant = Info->map[Game->my_ants[i]];
        for (j = 0; j < 4; ++j) {
            struct tile *neighbor = tileInDirection(directions[j], &my_ant, Info, Game);
            if (neighbor->state != WATER) {
                for (k = 0; k < Info->attack_offset_length; ++k) {
                    int row = (Info->attack_offsets_sq[k][0] + neighbor->row) % Info->rows;
                    int col = (Info->attack_offsets_sq[k][1] + neighbor->col) % Info->cols;
                    if (row < 0) row = Info->rows + row;
                    if (col < 0) col = Info->cols + col;
                    struct tile *tile = &Info->map[row*Info->cols + col];
                    if (tile->my_attack_influence == 0)
                        ++num_attack_tiles;
                    ++tile->my_attack_influence;
                }
            }
        }
    }
    // count how many times all enemy ants could affect each tile after moving one tile
    for (i = 0; i < Game->enemy_count; ++i) {
        struct tile enemy_ant = Info->map[Game->enemy_ants[i]];
        for (j = 0; j < 4; ++j) {
            struct tile *neighbor = tileInDirection(directions[j], &enemy_ant, Info, Game);
            if (neighbor->state != WATER) {
                for (k = 0; k < Info->attack_offset_length; ++k) {
                    int row = (Info->attack_offsets_sq[k][0] + neighbor->row) % Info->rows;
                    int col = (Info->attack_offsets_sq[k][1] + neighbor->col) % Info->cols;
                    if (row < 0) row = Info->rows + row;
                    if (col < 0) col = Info->cols + col;
                    struct tile *tile = &Info->map[row*Info->cols + col];
                    if (tile->enemy_attack_influence == 0)
                        ++num_attack_tiles;
                    ++tile->enemy_attack_influence;
                }
            }
        }
    }
    // find the best enemy ant, i.e. the one that is fighting the least
    // number of other ants, for every tile I could potentially move into.
    // then we can compore my weakness (the number of ants I'll be fighting)
    // to the best enemy's weakness (the number of ants it will be fighting)
    for (i = 0; i < Game->my_count; ++i) {
        struct tile my_ant = Info->map[Game->my_ants[i]];
        for (j = 0; j < 4; ++j) {
            struct tile *neighbor = tileInDirection(directions[j], &my_ant, Info, Game);
            if (neighbor->state != WATER) {
                int fighting = 100;                
                for (k = 0; k < Info->attack_offset_length; ++k) {
                    int row = (Info->attack_offsets_sq[k][0] + neighbor->row) % Info->rows;
                    int col = (Info->attack_offsets_sq[k][1] + neighbor->col) % Info->cols;
                    if (row < 0) row = Info->rows + row;
                    if (col < 0) col = Info->cols + col;
                    struct tile *tile = &Info->map[row*Info->cols + col];
                    if (tile->my_attack_influence < fighting)
                        fighting = tile->my_attack_influence;
                }
                if (neighbor->enemy_attack_influence < fighting)
                    neighbor->combat = SAFE;
                else if (neighbor->enemy_attack_influence == fighting)
                    neighbor->combat = KILL;
                else
                    neighbor->combat = DIE;
            }
        }
    }
    
}
