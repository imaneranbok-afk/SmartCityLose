#include <iostream>
#include <cassert>
#include "geometry/StraightGeometry.h"
#include "geometry/CurvedGeometry.h"
#include "geometry/RoundaboutGeometry.h"
#include "raymath.h"

void test_straight_geometry() {
    Vector3 start = {0, 0, 0};
    Vector3 end = {100, 0, 0};
    StraightGeometry geom(start, end, 10.0f, 2);
    
    assert(geom.GetWidth() == 10.0f);
    assert(geom.GetLength() == 100.0f);
    assert(geom.GetCenter().x == 50.0f);
    
    auto points = geom.GetPoints();
    assert(points.size() >= 2);
    
    std::cout << "StraightGeometry tests passed!" << std::endl;
}

void test_curved_geometry() {
    Vector3 p0 = {0, 0, 0};
    Vector3 p1 = {50, 0, 50};
    Vector3 p2 = {150, 0, 50};
    Vector3 p3 = {200, 0, 0};
    CurvedGeometry geom(p0, p1, p2, p3, 10.0f);
    
    assert(geom.GetWidth() == 10.0f);
    assert(geom.GetLength() > 200.0f); // Curved is longer than straight
    
    std::cout << "CurvedGeometry tests passed!" << std::endl;
}

void test_roundabout_geometry() {
    Vector3 center = {0, 0, 0};
    RoundaboutGeometry geom(center, 50.0f, 20.0f);
    
    assert(geom.GetOuterRadius() == 50.0f);
    assert(geom.GetWidth() == 20.0f);
    assert(geom.GetCenter().x == 0.0f);
    
    std::cout << "RoundaboutGeometry tests passed!" << std::endl;
}

int main() {
    std::cout << "Running GeometryStrategy tests..." << std::endl;
    test_straight_geometry();
    test_curved_geometry();
    test_roundabout_geometry();
    std::cout << "All GeometryStrategy tests passed!" << std::endl;
    return 0;
}
