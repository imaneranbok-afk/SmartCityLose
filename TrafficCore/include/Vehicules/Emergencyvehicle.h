#ifndef EMERGENCY_VEHICLE_H
#define EMERGENCY_VEHICLE_H

#include "Vehicule.h"
#include "Emergencymanager.h" // For EmergencyType enum

class RoadNetwork;
class Node;

class EmergencyVehicle : public Vehicule {
private:
    EmergencyType emergencyType;
    RoadNetwork* network;
    bool isOnEmergencyMission;
    bool isSirenActive;
    float sirenTimer = 0.0f;
    Node* destinationNode;

public:
    EmergencyVehicle(Vector3 pos, EmergencyType type, Model model, RoadNetwork* network);
    
    void update(float deltaTime) override;
    void draw() override;
    
    void setEmergencyMission(Node* destination);
    bool isOnMission() const { return isOnEmergencyMission; }
    bool hasReachedDestination() const;
    void completeMission();
    
    EmergencyType getType() const;

private:
    Node* findNearestNode() const;
    
    // Helpers for procedural realistic drawing
    void drawAmbulance();
    void drawPoliceCar();
    void drawFireTruck();
    void drawSiren(Vector3 localPos);
};

#endif