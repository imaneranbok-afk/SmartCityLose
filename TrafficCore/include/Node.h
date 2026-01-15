#ifndef NODE_H
#define NODE_H

#include "raylib.h"
#include <vector>

enum NodeType { 
    SIMPLE_INTERSECTION, 
    ROUNDABOUT, 
    TRAFFIC_LIGHT 
};

enum TrafficLightState {
    LIGHT_RED,
    LIGHT_YELLOW,
    LIGHT_GREEN
};

class Node {
private:
    int id;
    Vector3 position;
    NodeType type;
    float radius;
    std::vector<class RoadSegment*> connectedRoads;
    
    // Gestion des feux de circulation
    TrafficLightState lightState;
    float lightTimer;
    float redDuration;
    float yellowDuration;
    float greenDuration;
    bool emergencyOverride;  // Force le feu au vert pour les urgences
    float emergencyOverrideTimer;
    
public:
    Node(int id, Vector3 position, NodeType type = SIMPLE_INTERSECTION, float radius = 5.0f);
    
    // Getters
    int GetId() const { return id; }
    Vector3 GetPosition() const { return position; }
    NodeType GetType() const { return type; }
    float GetRadius() const { return radius; }
    const std::vector<RoadSegment*>& GetConnectedRoads() const { return connectedRoads; }
    TrafficLightState GetLightState() const { return lightState; }
    bool IsGreen() const { return lightState == LIGHT_GREEN || emergencyOverride; }
    bool HasEmergencyOverride() const { return emergencyOverride; }
    
    // Setters
    void SetPosition(Vector3 pos) { position = pos; }
    
    // Gestion des connexions
    void AddConnectedRoad(RoadSegment* road);
    
    // Pour calculer les tangentes aux connexions (utile pour les courbes)
    Vector3 GetConnectionTangent(Vector3 direction) const;
    
    // Gestion des feux de circulation
    void UpdateTrafficLight(float deltaTime);
    void SetEmergencyOverride(bool override, float duration = 5.0f);
    
    void Draw() const;
};

#endif