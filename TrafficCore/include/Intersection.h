#ifndef INTERSECTION_H
#define INTERSECTION_H

#include "Node.h"
#include "geometry/RoundaboutGeometry.h"
#include <memory>

class Vehicule;

class Intersection {
public:
    Intersection(Node* node);
    void Draw() const;
    bool CanEnter(Vehicule* vehicle) const;
    void Update(float deltaTime);

private:
    Node* node;
    std::unique_ptr<RoundaboutGeometry> roundaboutGeometry;
};

#endif // INTERSECTION_H
