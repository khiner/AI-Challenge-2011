from ants import *
from MyBot import *
import heapq

class Ant:
    def __init__(self, ants, loc):
        self.ants = ants
        self.loc = loc
        self.dest = None
        self.path = []

    def setPath(self, path):
        self.path = path

    def moveToNext(self):
        nextLoc = self.path[-1] # peek
        if self.do_move_direction(self.ants.direction(self.loc, nextLoc.loc)[0]):
            self.path.pop() # if the move w<orked, pop

    def do_move_direction(self, direction):
        new_loc = self.ants.destination(self.loc, direction)
        if self.ants.unoccupied(new_loc) and new_loc not in self.ants.orders:
            self.ants.issue_order((self.loc, direction))
            self.ants.orders[new_loc] = self.loc
            self.loc = new_loc
            return True
        else:
            return False

    def do_move_location(self, dest):
        if not self.dest:
            self.dest = dest
            self.path = self.aStar(self.ants.graph, self.ants.nodes[self.loc], self.ants.nodes[self.dest])
            if not self.path:
                self.dest = None
        
    def aStar(self, graph, source, dest):
        sys.stderr.write('astaring\n')
        sys.stderr.flush()
        if source == dest:
            return []
        openSet = set()
        openHeap = []
        closedSet = set()

        def retracePath(c):
            path = [c]
            while c.parent is not None:
                c = c.parent
                path.append(c)
            # path.reverse() can we pop through path w/o this?
            path.pop()
            return path

        openSet.add(source)
        openHeap.append((0,source))
        while openSet:
            source = heapq.heappop(openHeap)[1]
            if source == dest:
                return retracePath(source)
            openSet.remove(source)
            closedSet.add(source)
            for node in graph[source]:
                if node not in closedSet:
                    node.H = self.ants.distance(node.loc, dest.loc)
                    if node not in openSet:
                        openSet.add(node)
                        heapq.heappush(openHeap, (node.H,node))
                    node.parent = source
        return []
            