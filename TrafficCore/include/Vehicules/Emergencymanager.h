#ifndef EMERGENCY_MANAGER_H
#define EMERGENCY_MANAGER_H

#include "raylib.h"
#include "raymath.h"
#include "RoadNetwork.h"
#include "Node.h"
#include "Vehicules/ModelManager.h"
#include "Vehicules/Vehicule.h"
#include <vector>
#include <algorithm>

class RoadNetwork;
class EmergencyVehicle;

enum EmergencyType {
    AMBULANCE = 0,
    FIRE_TRUCK = 1,
    POLICE = 2
};

struct Hospital {
    Vector3 position;
    Node* entryNode;
};

class EmergencyManager {
private:
    Hospital hospital;
    std::vector<EmergencyVehicle*> emergencyVehicles;
    RoadNetwork* network;

public:
    EmergencyManager(RoadNetwork* net);

    // Place l'hôpital et lie au noeud le plus proche
    void addHospital(Vector2 pos2D);

    // Déclenche une intervention (Vector2 pour compatibilité)
    void dispatchEmergencyVehicle(int vehicleType, Vector2 destination);
    
    // Déclenche une intervention (Vector3)
    void dispatchEmergencyVehicle(int vehicleType, Vector3 destination);

    // Gestion des feux adaptatifs
    void updateTrafficLights(float deltaTime);

    // Interaction : les voitures normales s'écartent ou s'arrêtent
    void yieldToEmergencyVehicle(std::vector<Vehicule*>& normalTraffic);

    // Mise à jour et rendu
    void updateAndDraw(float dt);
    void update(float deltaTime);
    
    // Accesseurs
    std::vector<EmergencyVehicle*>& getEmergencyVehicles();
    const Hospital& getHospital() const;
};

#endif