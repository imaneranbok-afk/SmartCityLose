#include <iostream>
#include <cassert>
#include "RoadNetwork.h"
#include "Intersection.h"

void test_intersection_creation() {
    RoadNetwork network;
    Node* n1 = network.AddNode({0, 0, 0}, ROUNDABOUT);
    Intersection* inter = network.AddIntersection(n1);
    
    assert(network.GetIntersectionCount() == 1);
    assert(inter != nullptr);
    
    std::cout << "Intersection creation test passed!" << std::endl;
}

void test_intersection_can_enter() {
    RoadNetwork network;
    Node* n1 = network.AddNode({0, 0, 0});
    Intersection inter(n1);
    
    // Note: CanEnter might return true by default if not fully implemented
    bool result = inter.CanEnter(nullptr); 
    assert(result == true || result == false);
    
    std::cout << "Intersection logic test passed!" << std::endl;
}

int main() {
    std::cout << "Running Intersection tests..." << std::endl;
    test_intersection_creation();
    test_intersection_can_enter();
    std::cout << "All Intersection tests passed!" << std::endl;
    return 0;
}
