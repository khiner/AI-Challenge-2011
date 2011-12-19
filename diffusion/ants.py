#!/usr/bin/env python
import sys
import traceback
import random
import time
from collections import defaultdict
from math import sqrt
from numpy import zeros

MY_ANT = 0
ANTS = 0
DEAD = -1
LAND = -2
FOOD = -3
WATER = -4

MAX = 255

EXPLORE_GOAL = 0
FOOD_GOAL = 1
HILL_GOAL = 2
ROAM_GOAL = 3
ANTI_CLUSTER_GOAL = 4

ENEMY_HILL = 6

PLAYER_ANT = 'abcdefghij'
HILL_ANT = string = 'ABCDEFGHIJ'
PLAYER_HILL = string = '0123456789'
MAP_OBJECT = '?%*.!'
MAP_RENDER = PLAYER_ANT + HILL_ANT + PLAYER_HILL + MAP_OBJECT

AIM = {'n': (-1, 0),
       'e': (0, 1),
       's': (1, 0),
       'w': (0, -1)}
RIGHT = {'n': 'e',
         'e': 's',
         's': 'w',
         'w': 'n'}
LEFT = {'n': 'w',
        'e': 'n',
        's': 'e',
        'w': 's'}
BEHIND = {'n': 's',
          's': 'n',
          'e': 'w',
          'w': 'e'}

class Ants():
    def __init__(self):
        self.tiles = None
        self.cols = None
        self.rows = None
        self.map = {}
        self.hill_list = {}
        self.ant_list = {}
        self.turntime = 0
        self.loadtime = 0
        self.turn_start_time = None
        self.vision = None
        self.viewradius2 = 0
        self.attackradius2 = 0
        self.spawnradius2 = 0
        self.turns = 0
        self.turn = 0

    def setup(self, data):
        'parse initial input and setup starting game state'
        for line in data.split('\n'):
            line = line.strip().lower()
            if len(line) > 0:
                tokens = line.split()
                key = tokens[0]
                if key == 'cols':
                    self.cols = int(tokens[1])
                elif key == 'rows':
                    self.rows = int(tokens[1])
                elif key == 'player_seed':
                    random.seed(int(tokens[1]))
                elif key == 'turntime':
                    self.turntime = int(tokens[1])
                elif key == 'loadtime':
                    self.loadtime = int(tokens[1])
                elif key == 'viewradius2':
                    self.viewradius2 = int(tokens[1])
                elif key == 'attackradius2':
                    self.attackradius2 = int(tokens[1])
                elif key == 'spawnradius2':
                    self.spawnradius2 = int(tokens[1])
                elif key == 'turns':
                    self.turns = int(tokens[1])
        for row in xrange(self.rows):
            for col in xrange(self.cols):
                self.map[(row, col)] = Tile(self, (row, col), LAND)

    def updateVision(self):
        ' determine which tiles are visible to the given player '
        if self.vision == None:
            if not hasattr(self, 'vision_offsets_2'):
                # precalculate tiles around an ant to set as visible
                self.vision_offsets_2 = []
                mx = int(sqrt(self.viewradius2))
                for d_row in range(-mx,mx+1):
                    for d_col in range(-mx,mx+1):
                        d = d_row**2 + d_col**2
                        if d <= self.viewradius2:
                            self.vision_offsets_2.append((
                                # Create all negative offsets so vision will
                                # wrap around the edges properly
                                (d_row % self.rows) - self.rows,
                                (d_col % self.cols) - self.cols
                            ))
            # set all spaces as not visible
            # loop through ants and set all tiles around ant as visible
            self.vision = [[False]*self.cols for row in range(self.rows)]
            for ant in self.my_ants():
                a_row, a_col = ant
                for v_row, v_col in self.vision_offsets_2:
                    self.vision[a_row + v_row][a_col + v_col] = True
        
    def update(self, data):
        'parse engine input and update the game state'
        self.turn = self.turn + 1
        
        # reset vision
        self.vision = None
        
        # clear hill, ant and food data
        for tile in self.map.values():
            if tile.state != WATER:
                tile.state = LAND
        self.hill_list = {}                    
        self.ant_list = {}
        
        # update map and create new ant and food lists
        for line in data.split('\n'):
            line = line.strip().lower()
            if len(line) > 0:
                tokens = line.split()
                if len(tokens) >= 3:
                    row = int(tokens[1])
                    col = int(tokens[2])
                    if tokens[0] == 'w':
                        self.map[(row, col)].state = WATER
                    elif tokens[0] == 'f':
                        self.map[(row, col)].state = FOOD
                    else:
                        owner = int(tokens[3])
                        if tokens[0] == 'a':
                            self.ant_list[(row, col)] = owner
                            self.map[(row, col)].state = MY_ANT
                        elif tokens[0] == 'd':
                            # food could spawn on a spot where an ant just died
                            # don't overwrite the space unless it is land
                            if self.map[(row, col)].state == LAND:
                                self.map[(row, col)].state = DEAD
                        elif tokens[0] == 'h':
                            owner = int(tokens[3])
                            self.hill_list[(row, col)] = owner
                            if owner != 0:
                                self.map[(row, col)].state = ENEMY_HILL
        self.updateVision()
        for x in xrange(3):
            for tile in self.map.values():
                tile.diffusion()
                            
    def issue_order(self, order):
        'issue an order by writing the proper ant location and direction'
        (row, col), direction = order
        sys.stdout.write('o %s %s %s\n' % (row, col, direction))
#        for row in range(0, self.rows):
#            for col in range(0, self.cols):
#                sys.stdout.write('v sfc 255 0 0 %s\n' % (self.map[(row, col)].agents[HILL_GOAL]))
#                sys.stdout.write('v tile %s %s\n' % (row, col))
        sys.stdout.flush()        
        
    def finish_turn(self):
        'finish the turn by writing the go line'
        sys.stdout.write('go\n')
        sys.stdout.flush()
    
    def my_hills(self):
        return [loc for loc, owner in self.hill_list.items()
                    if owner == MY_ANT]

    def enemy_hills(self):
        return [(loc, owner) for loc, owner in self.hill_list.items()
                    if owner != MY_ANT]
        
    def my_ants(self):
        'return a list of all my ants'
        return [(row, col) for (row, col), owner in self.ant_list.items()
                    if owner == MY_ANT]

    def enemy_ants(self):
        'return a list of all visible enemy ants'
        return [((row, col), owner)
                    for (row, col), owner in self.ant_list.items()
                    if owner != MY_ANT]

    def passable(self, loc):
        'true if not water'
        return self.map[loc].state != WATER
    
    def unoccupied(self, loc):
        'true if no ants are at the location'
        return self.map[loc].state != MY_ANT

    def destination(self, loc, direction):
        'calculate a new location given the direction and wrap correctly'
        row, col = loc
        d_row, d_col = AIM[direction]
        return ((row + d_row) % self.rows, (col + d_col) % self.cols)       

    def distance(self, loc1, loc2):
        'calculate the closest distance between to locations'
        row1, col1 = loc1
        row2, col2 = loc2
        d_col = min(abs(col1 - col2), self.cols - abs(col1 - col2))
        d_row = min(abs(row1 - row2), self.rows - abs(row1 - row2))
        return d_row + d_col

    def direction(self, loc1, loc2):
        'determine the 1 or 2 fastest (closest) directions to reach a location'
        row1, col1 = loc1
        row2, col2 = loc2
        height2 = self.rows//2
        width2 = self.cols//2
        d = []
        if row1 < row2:
            if row2 - row1 >= height2:
                d.append('n')
            if row2 - row1 <= height2:
                d.append('s')
        if row2 < row1:
            if row1 - row2 >= height2:
                d.append('s')
            if row1 - row2 <= height2:
                d.append('n')
        if col1 < col2:
            if col2 - col1 >= width2:
                d.append('w')
            if col2 - col1 <= width2:
                d.append('e')
        if col2 < col1:
            if col1 - col2 >= width2:
                d.append('e')
            if col1 - col2 <= width2:
                d.append('w')
        return d

    def visible(self, loc):
        return self.vision[loc[0]][loc[1]]
    
    def upTile(self, tile):
        return self.map.get(self.destination(tile.loc, 'n'))

    def downTile(self, tile):
        return self.map.get(self.destination(tile.loc, 's'))

    def leftTile(self, tile):
        return self.map.get(self.destination(tile.loc, 'w'))

    def rightTile(self, tile):
        return self.map.get(self.destination(tile.loc, 'e'))

    def valueOfFoodAt(self, pos):
        return self.map.get(pos).agents[FOOD_GOAL]

    def valueOfHillAt(self, pos):
        return self.map.get(pos).agents[HILL_GOAL]

    def valueOfExploreAt(self, pos):
        return self.map.get(pos).agents[EXPLORE_GOAL]

    def valueOfRoamAt(self, pos):
        return self.map.get(pos).agents[ROAM_GOAL]
        
    # static methods are not tied to a class and don't have self passed in
    # this is a python decorator
    @staticmethod
    def run(bot):
        'parse input, update game state and call the bot classes do_turn method'
        ants = Ants()
        map_data = ''
        while(True):
            try:
                current_line = sys.stdin.readline().rstrip('\r\n') # string new line char
                if current_line.lower() == 'ready':
                    ants.setup(map_data)
                    bot.do_setup(ants)
                    ants.finish_turn()
                    map_data = ''
                elif current_line.lower() == 'go':
                    ants.update(map_data)
                    # call the do_turn method of the class passed in
                    bot.do_turn(ants)
                    ants.finish_turn()
                    map_data = ''
                else:
                    map_data += current_line + '\n'
            except EOFError:
                break
            except KeyboardInterrupt:
                raise
            except:
                # don't raise error or return so that bot attempts to stay alive
                traceback.print_exc(file=sys.stderr)
                sys.stderr.flush()

class Tile:
    def __init__(self, ants, loc, state):
        self.ants = ants
        self.loc = loc
        self.state = state
        self.agents = zeros(4)
        self.lastSeen = 0
        self.seen = False

    def markSeen(self):
        self.lastSeen = self.ants.turn
        self.seen = True

    def clearDiffusion(self):
        self.agents = zeros(4)
        
    def diffusion(self):
        goalsToDiffuse = []
        # water blocks everything
        if self.state == WATER:
            self.clearDiffusion()
            return
            
        # FOOD
        if self.state == FOOD:
            self.agents[FOOD_GOAL] = MAX
        else:
            goalsToDiffuse.append(FOOD_GOAL)
            
        if self.state == ENEMY_HILL:
            self.agents[HILL_GOAL] = MAX
        else:
            goalsToDiffuse.append(HILL_GOAL)

        # EXPLORE        
        if self.ants.visible(self.loc):
            self.lastSeen = self.ants.turn
            if not self.seen:
                self.seen = True
#            goalsToDiffuse.append(ROAM_GOAL)
#        else:
#            self.agents[ROAM_GOAL] = MAX - self.lastSeen
            
        if self.seen:
            goalsToDiffuse.append(EXPLORE_GOAL)
        else:
            self.agents[EXPLORE_GOAL] = MAX
            
        for goal in goalsToDiffuse:
            up = self.ants.upTile(self).agents[goal]
            down = self.ants.downTile(self).agents[goal]
            left = self.ants.leftTile(self).agents[goal]
            right = self.ants.rightTile(self).agents[goal]
            self.agents[goal] = 0.25*(up + down + left + right)