#!/usr/bin/env python
from ants import *

class MyBot:
    def __init__(self):
        # define class level variables, will be remembered between turns
        pass
    
    # do_setup is run once at the start of the game
    # after the bot has received the game settings
    # the MyAnts class is created and setup by the MyAnts.run method
    def do_setup(self, ants):
        self.hills = []
                
    # do turn is run once per turn
    def do_turn(self, ants):
        orders = {}
        def do_move_direction(loc, direction):
            new_loc = ants.destination(loc, direction)
            if (ants.unoccupied(new_loc) and ants.passable(new_loc) and new_loc not in orders):
                ants.issue_order((loc, direction))
                orders[new_loc] = loc
                return True
            else:
                return False
                
        targets = {}
        def do_move_location(loc, dest):
            directions = ants.direction(loc, dest)
            for direction in directions:
                if do_move_direction(loc, direction):
                    targets[dest] = loc
                    return True
            return False

        def outputMove(ant, goal):
            highestVal = -1
            bestDirection = None
            for direction in ('s','e','w','n'):
                loc = ants.destination(ant, direction)
                val = ants.map.get(loc).agents[goal]
                if val > highestVal:
                    highestVal = val
                    bestDirection = direction
            if bestDirection:
                do_move_direction(ant, bestDirection)

            
        # prevent stepping on own hill
        for hill_loc in ants.my_hills():
            orders[hill_loc] = None

        ant_list = ants.my_ants()
        for i in xrange(len(ant_list)):
            ant = ant_list[i]
            hill = ants.valueOfHillAt(ant)
            food = ants.valueOfFoodAt(ant)
            explore = ants.valueOfExploreAt(ant)
            roam = ants.valueOfRoamAt(ant)

            if hill != 0:
                goal = HILL_GOAL
            elif food != 0:
                goal = FOOD_GOAL
            else:
                goal = EXPLORE_GOAL
            
            outputMove(ant, goal)
                    
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
