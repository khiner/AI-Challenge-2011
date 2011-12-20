#include "ants.h"

// returns the absolute value of a number; used in distance function

int abs(int x) {
    if (x >= 0)
        return x;
    return -x;
}

// returns the distance between two items on the grid accounting for map wrapping

int distance(int row1, int col1, int row2, int col2, struct game_info *Info) {
    int dr, dc;
    int abs1, abs2;

    abs1 = abs(row1 - row2);
    abs2 = Info->rows - abs(row1 - row2);

    if (abs1 > abs2)
        dr = abs2;
    else
        dr = abs1;

    abs1 = abs(col1 - col2);
    abs2 = Info->cols - abs(col1 - col2);

    if (abs1 > abs2)
        dc = abs2;
    else
        dc = abs1;

    return sqrt(pow(dr, 2) + pow(dc, 2));
}

// sends a move to the tournament engine and keeps track of ants new location

void move(struct tile *ant, char dir, struct game_info* Info, struct game_state* Game) {
    if (ant->state == MY_ANT || ant->state == MY_ANT_AND_HILL) // sanity check
        fprintf(stdout, "O %i %i %c\n", ant->row, ant->col, dir);
    else
        fprintf(stderr, "oops! tried moving non-ant tile!\n");        
}

// just a function that returns the string on a given line for i/o
// you don't need to worry about this

char *get_line(char *text) {
    char *tmp_ptr = text;
    int len = 0;

    while (*tmp_ptr != '\n') {
        ++tmp_ptr;
        ++len;
    }

    char *return_str = malloc(len + 1);
    memset(return_str, 0, len + 1);

    int i = 0;
    for (; i < len; ++i) {
        return_str[i] = text[i];
    }

    return return_str;
}

void show_debug(struct game_info *Info) {
    int i;
    for (i = 0; i < Info->rows*Info->cols; ++i) {
        struct tile tile = Info->map[i];
        // food = RED
        fprintf(stdout, "v sfc %d %d %d %f\n", 255, 0, 0, tile.agents[FOOD_GOAL]);
        fprintf(stdout, "v tile %d %d\n", tile.row, tile.col);
        // explore = GREEN
        fprintf(stdout, "v sfc %d %d %d %f\n", 0, 255, 0, tile.agents[EXPLORE_GOAL]);
        fprintf(stdout, "v tile %d %d\n", tile.row, tile.col);
    }        
}

// main, communicates with tournament engine

int main(int argc, char *argv[]) {
    int action = -1;

    struct game_info Info;
    struct game_state Game;
    Info.map = 0;

    Game.my_ants = 0;
    Game.enemy_ants = 0;
    Game.my_hills = 0;
    Game.enemy_hills = 0;
    
    while (42) {
        int initial_buffer = 100000;

        char *data = malloc(initial_buffer);
        memset(data, 0, initial_buffer);

        *data = '\n';

        char *ins_data = data + 1;

        int i = 0;

        while (1 > 0) {
            ++i;

            if (i >= initial_buffer) {
                initial_buffer *= 2;
                data = realloc(data, initial_buffer);
                ins_data = data + i;
                memset(ins_data, 0, initial_buffer - i);
            }

            *ins_data = getchar();

            if (*ins_data == '\n') {
                char *backup = ins_data;

                while (*(backup - 1) != '\n') {
                    --backup;
                }

                char *test_cmd = get_line(backup);

                if (strcmp(test_cmd, "go") == 0) {
                    action = 0; 
                    free(test_cmd);
                    break;
                }
                else if (strcmp(test_cmd, "ready") == 0) {
                    action = 1;
                    free(test_cmd);
                    break;
                }
                free(test_cmd);
            }
            
            ++ins_data;
        }

        if (action == 0) {
            char *skip_line = data + 1;
            while (*++skip_line != '\n');
            ++skip_line;

            _init_map(skip_line, &Info, &Game);
            _init_game(&Info, &Game);
            Info.curr_turn++;
            updateVision(&Info, &Game);
            int i;
            for (i= 0; i < NUM_DIFFUSIONS; ++i)
                diffuseAll(&Info, &Game);
            show_debug(&Info);
            do_turn(&Game, &Info);
            fprintf(stdout, "go\n");
            fflush(stdout);
        }
        else if (action == 1) {
            _init_ants(data + 1, &Info);
            Game.my_ant_index = -1;

            fprintf(stdout, "go\n");
            fflush(stdout);
        }
        free(data);
    }
}
