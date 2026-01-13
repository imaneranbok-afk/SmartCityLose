#ifndef PATHFINDER_H
#define PATHFINDER_H

#include <vector>
#include "Node.h"

class RoadNetwork;

// PathFinder : implémentation A* simple mais extensible.
// - Respecte le graphe (Noeuds = intersections, Arêtes = RoadSegment)
// - Coût principal : longueur de segment
// - Coût additionnel : préférence pour routes larges (plus de voies)
// - API courte : conserve FindPath(start,end)
class PathFinder {
public:
    explicit PathFinder(const RoadNetwork* network);
    std::vector<Node*> FindPath(Node* start, Node* end) const;

private:
    const RoadNetwork* network;
};

#endif // PATHFINDER_H
