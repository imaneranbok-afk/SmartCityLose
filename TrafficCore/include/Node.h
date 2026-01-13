#ifndef NODE_H
#define NODE_H

#include "raylib.h"
#include <vector>

enum NodeType { 
    SIMPLE_INTERSECTION, 
    ROUNDABOUT, 
    TRAFFIC_LIGHT 
};

class Node {
private:
    int id;
    Vector3 position;
    NodeType type;
    float radius;
    std::vector<class RoadSegment*> connectedRoads;
    
public:
    Node(int id, Vector3 position, NodeType type = SIMPLE_INTERSECTION, float radius = 5.0f);
    
    // Getters
    int GetId() const { return id; }
    Vector3 GetPosition() const { return position; }
    NodeType GetType() const { return type; }
    float GetRadius() const { return radius; }
    const std::vector<RoadSegment*>& GetConnectedRoads() const { return connectedRoads; }
    
    // Setters
    void SetPosition(Vector3 pos) { position = pos; }
    
    // Gestion des connexions
    void AddConnectedRoad(RoadSegment* road);
    
    // Pour calculer les tangentes aux connexions (utile pour les courbes)
    Vector3 GetConnectionTangent(Vector3 direction) const;
    
    void Draw() const;
};

#endif