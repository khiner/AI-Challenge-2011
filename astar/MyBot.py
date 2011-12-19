#!/usr/bin/env python
from ants import *
from Ant import *
import math

# define a class with a do_turn method
# the Ants.run method will parse and update bot input
# it will also run the do_turn method for us
class MyBot:
    def __init__(self):
        self.ant_list = [] # list of ants
 
    # do_setup is run once at the start of the game
    # after the bot has received the game settings
    # the ants class is created and setup by the Ants.run method
    def do_setup(self, ants):
        self.unseen = []
        self.hills = []
        for row in range(ants.rows):
            for col in range(ants.cols):
                node = Node((row, col))
                ants.nodes[(row, col)] = node
                ants.unfinished_nodes.append(node)
                
    # do turn is run once per turn
    def do_turn(self, ants):
        ants.orders.clear()
        ants.destinations.clear()
        # update ant list - my ants should be updated to reflect
        # where they were ordered to go last turn.
        # so any ants not in the ant_list are new.

        if len(ants.my_ants()) != len(self.ant_list):
            expected_locs = []
            for ant in self.ant_list:
                expected_locs.append(ant.loc)
        
            for loc in ants.my_ants():
                if loc not in expected_locs:
                    self.ant_list.append(Ant(ants, loc))

            # likewise, any dead ants should be removed.
            for loc in ants.dead_list:
                for ant in self.ant_list:            
                    if loc == ant.loc:
                        self.ant_list.remove(loc)

        sys.stderr.write('ants: ' + str(len(self.ant_list)) + '\n')                        
        # connect previously invisible ants.nodes to the graph
        # and remove ants.nodes that have all neighbors mapped from unfinished
        for node in ants.unfinished_nodes[:]:
            if ants.visible(node.loc):
                finished = True
                for neighbor_loc in ants.neighbors(node.loc):
                    if ants.visible(neighbor_loc):
                        neighbor_node = ants.nodes[neighbor_loc]
                        if not node in ants.graph.keys():
                            ants.graph[node] = set([neighbor_node])
                        else:
                            ants.graph[node].add(neighbor_node)
                    else:
                        finished = False
                if finished:
                    ants.unfinished_nodes.remove(node)

        # add all ants.destinations
        for food_loc in ants.food():
            ants.destinations.add(food_loc)
        for hill_loc, hill_owner in ants.enemy_hills():
            ants.destinations.add(hill_loc)
        for unseen_loc in self.unseen:
            ants.destinations.add(unseen_loc)
            
        ant_dist = []
        for dest in ants.destinations:
            for ant in self.ant_list:
                dist = ants.distance(ant.loc, dest)
                ant_dist.append((dist, ant, dest))
        ant_dist.sort()
        for dist, ant, dest in ant_dist:
            ant.do_move_location(dest)

        # move all ants to their next locitions
        for ant in self.ant_list:
            ant.moveToNext() 
                                        
if __name__ == '__main__':
    # psyco will speed up python a little, but is not needed
    try:
        import psyco
        psyco.full()
    except ImportError:
        pass
    
    try:
        # if run is passed a class with a do_turn method, it will do the work
        # this is not needed, in which case you will need to write your own
        # parsing function and your own game state class
        Ants.run(MyBot())
    except KeyboardInterrupt:
        print('ctrl-c, leaving ...')

class Node:
    def __init__(self, loc):
        self.loc = loc
        self.parent = None
        self.H = 0

    def __hash__(self):
        return hash(self.loc)
