#include <iostream>
#include <cassert>
#include "RoadNetwork.h"
#include "PathFinder.h"

void test_nodes() {
    RoadNetwork network;
    Node* n1 = network.AddNode({0, 0, 0});
    Node* n2 = network.AddNode({100, 0, 0});
    
    assert(network.GetNodeCount() == 2);
    assert(n1->GetId() == 1);
    assert(n2->GetId() == 2);
    assert(network.FindNodeById(1) == n1);
    assert(network.FindNodeById(2) == n2);
    
    std::cout << "Node tests passed!" << std::endl;
}

void test_segments() {
    RoadNetwork network;
    Node* n1 = network.AddNode({0, 0, 0});
    Node* n2 = network.AddNode({100, 0, 0});
    RoadSegment* segment = network.AddRoadSegment(n1, n2, 2);
    
    assert(network.GetRoadSegmentCount() == 1);
    assert(segment->GetStartNode() == n1);
    assert(segment->GetEndNode() == n2);
    assert(segment->GetLanes() == 2);
    
    std::cout << "Segment tests passed!" << std::endl;
}

void test_pathfinding() {
    RoadNetwork network;
    Node* n1 = network.AddNode({0, 0, 0});
    Node* n2 = network.AddNode({100, 0, 0});
    Node* n3 = network.AddNode({200, 0, 0});
    
    network.AddRoadSegment(n1, n2, 2);
    network.AddRoadSegment(n2, n3, 2);
    
    PathFinder pf(&network);
    auto path = pf.FindPath(n1, n3);
    assert(path.size() == 3);
    assert(path[0] == n1);
    assert(path[1] == n2);
    assert(path[2] == n3);
    
    std::cout << "Pathfinding tests passed!" << std::endl;
}

int main() {
    std::cout << "Running RoadNetwork tests..." << std::endl;
    test_nodes();
    test_segments();
    test_pathfinding();
    std::cout << "All RoadNetwork tests passed!" << std::endl;
    return 0;
}
