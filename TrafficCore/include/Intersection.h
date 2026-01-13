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
    // Gestion de l'occupation
    bool CanEnter(Vehicule* vehicle) const;
    void Enter(Vehicule* v);
    void Exit(Vehicule* v);
    
    bool IsOccupied() const { return !occupants.empty(); }
    void Update(float deltaTime);
    
    // Accesseurs
    Node* GetNode() const { return node; }
    const std::vector<Vehicule*>& GetOccupants() const { return occupants; }

private:
    Node* node;
    std::unique_ptr<RoundaboutGeometry> roundaboutGeometry;
    std::vector<Vehicule*> occupants; // Véhicules actuellement dans l'intersection
};

#endif // INTERSECTION_H
