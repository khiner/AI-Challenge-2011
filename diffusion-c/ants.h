#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

extern const int NUM_AGENTS;
extern const int NUM_DIFFUSIONS;
extern const int MAX;

extern const char FOOD;
extern const char WATER;
extern const char LAND;
extern const char DEAD;
extern const int ENEMY_HILL;
extern const int MY_HILL;
extern const char MY_ANT;
extern const char ENEMY_ANT;
extern const char MY_ANT_AND_HILL;
extern const char ENEMY_ANT_AND_HILL;
extern const int FOOD_GOAL;
extern const int HILL_GOAL;
extern const int EXPLORE_GOAL;

// this header is basically self-documenting

struct game_info {
	int loadtime;
	int turntime;
    int curr_turn;    
	int rows;
	int cols;
	int turns;
	int viewradius_sq;
	int attackradius_sq;
	int spawnradius_sq;
    int seed;
    int vision_offset_length;
    int **vision_offsets_sq;
	struct tile* map;
};

struct tile {
    char state;    
    int row;
    int col;
    int lastSeen;
    short seen;
    short visible;
    float agents[3];    
};

struct game_state {
    // arrays of indices into the game_info->map tiles for fast access
    int *my_ants;
    int *enemy_ants;
    int *my_hills;
    int *enemy_hills;

    int my_count;
    int enemy_count;
    int food_count;
    int dead_count;
    int my_hill_count;
    int enemy_hill_count;
    int my_ant_index;
};
