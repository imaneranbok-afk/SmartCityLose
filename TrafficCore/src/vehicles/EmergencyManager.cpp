#include "Vehicules/EmergencyManager.h"
#include "Vehicules/EmergencyVehicle.h"
#include "Vehicules/Vehicule.h"
#include "Vehicules/ModelManager.h"
#include "PathFinder.h"
#include "Node.h"
#include "RoadSegment.h"
#include <algorithm>
#include <cmath>

EmergencyManager::EmergencyManager(RoadNetwork* net) : network(net) {
    hospital.entryNode = nullptr;
    hospital.position = {0.0f, 0.0f, 0.0f};
}

void EmergencyManager::addHospital(Vector2 pos2D) {
    hospital.position = { pos2D.x, 0.0f, pos2D.y };
    
    // Clear existing vehicles to prevent duplicates and leaks
    for (auto* v : emergencyVehicles) {
        delete v;
    }
    emergencyVehicles.clear();
    
    if (!network) return;
    
    float minDist = 999999.0f;
    for (const auto& node : network->GetNodes()) {
        float d = Vector3Distance(hospital.position, node->GetPosition());
        if (d < minDist) {
            minDist = d;
            hospital.entryNode = node.get();
        }
    }

    // Create parking area next to the hospital
    // Spawn 3 stationary emergency vehicles (Ambulance, Fire, Police)
    Vector3 parkingStart = { hospital.position.x + 60.0f, 0.0f, hospital.position.z };
    
    for (int i = 0; i < 3; i++) {
        EmergencyType type = static_cast<EmergencyType>(i);
        Vector3 parkPos = { parkingStart.x + (i * 15.0f), 0.0f, parkingStart.z };
        
        Model m = ModelManager::getInstance().getRandomModel("EMERGENCY");
        // If m.meshCount == 0, EmergencyVehicle::draw() will handle procedural realistic rendering
        
        auto* ev = new EmergencyVehicle(parkPos, type, m, network);
        ev->completeMission(); // Ensure they are stationary/parked
        emergencyVehicles.push_back(ev);
    }
}

void EmergencyManager::dispatchEmergencyVehicle(int vehicleType, Vector2 destination) {
    if (!network) return;
    
    // Find a parked vehicle of the requested type
    EmergencyVehicle* ev = nullptr;
    for (size_t i = 0; i < emergencyVehicles.size(); i++) {
        // Assuming order 0=Ambulance, 1=Fire, 2=Police based on addHospital loop
        if (i % 3 == vehicleType) {
            if (!emergencyVehicles[i]->isOnMission()) {
                ev = emergencyVehicles[i];
                break;
            }
        }
    }
    
    if (!ev) return; // No vehicle available
    
    // Trouver le noeud de destination le plus proche
    Node* destNode = nullptr;
    float minDist = 999999.0f;
    Vector3 dest3D = {destination.x, 0.0f, destination.y};
    
    for (const auto& node : network->GetNodes()) {
        float d = Vector3Distance(dest3D, node->GetPosition());
        if (d < minDist) {
            minDist = d;
            destNode = node.get();
        }
    }
    
    if (!destNode) return;
    
    // Définir la mission d'urgence
    ev->setEmergencyMission(destNode);
}

void EmergencyManager::dispatchEmergencyVehicle(int vehicleType, Vector3 destination) {
    dispatchEmergencyVehicle(vehicleType, Vector2{destination.x, destination.z});
}

void EmergencyManager::updateTrafficLights(float deltaTime) {
    if (!network) return;
    
    for (const auto& node : network->GetNodes()) {
        if (node->GetType() == TRAFFIC_LIGHT && node->HasEmergencyOverride()) {
            continue;
        }
    }
    
    for (auto* ev : emergencyVehicles) {
        if (!ev->isOnMission()) continue;
        
        Vector3 evPos = ev->getPosition();
        
        for (const auto& node : network->GetNodes()) {
            if (node->GetType() != TRAFFIC_LIGHT) continue;
            
            float dist = Vector3Distance(evPos, node->GetPosition());
            float detectionRadius = 100.0f;

            if (ev->getType() == AMBULANCE || ev->getType() == FIRE_TRUCK) {
                detectionRadius = 200.0f;
            } else {
                detectionRadius = 80.0f;
            }
            
            if (dist < detectionRadius) { 
                Vector3 toLight = Vector3Subtract(node->GetPosition(), evPos);
                Vector3 evDir = {
                    sinf(ev->getRotationAngle() * DEG2RAD),
                    0,
                    cosf(ev->getRotationAngle() * DEG2RAD)
                };
                
                float dot = Vector3DotProduct(Vector3Normalize(toLight), evDir);
                
                if (dot > 0.0f) {
                    node->SetEmergencyOverride(true, 5.0f);
                }
            }
        }
    }
}

void EmergencyManager::yieldToEmergencyVehicle(std::vector<Vehicule*>& normalTraffic) {
    for (auto* v : normalTraffic) {
        if (dynamic_cast<EmergencyVehicle*>(v)) continue;

        bool mustYield = false;
        float closestDist = 9999.0f;

        for (auto* ev : emergencyVehicles) {
            if (!ev->isOnMission()) continue;

            Vector3 evPos = ev->getPosition();
            Vector3 evDir = {
                sinf(ev->getRotationAngle() * DEG2RAD),
                0,
                cosf(ev->getRotationAngle() * DEG2RAD)
            };

            Vector3 toVehicle = Vector3Subtract(v->getPosition(), evPos);
            float dist = Vector3Length(toVehicle);

            float dot = Vector3DotProduct(Vector3Normalize(toVehicle), evDir);

            if (dist < 80.0f && dot > 0.0f) {
                mustYield = true;
                if (dist < closestDist) closestDist = dist;
            }
        }

        if (mustYield) {
            if (v->getLane() != 9) {
                v->setLane(9);
            }

            if (closestDist < 30.0f) {
                v->setWaiting(true);
            } else {
                v->setWaiting(false);
            }
        } else {
            if (v->getLane() == 9) {
                v->setLane(0);
                v->setWaiting(false);
            }
        }
    }
}

void EmergencyManager::update(float deltaTime) {
    for (auto* ev : emergencyVehicles) {
        if (ev->hasReachedDestination()) {
            ev->completeMission();
        }
        ev->update(deltaTime);
    }
    
    updateTrafficLights(deltaTime);
}

void EmergencyManager::updateAndDraw(float dt) {
    update(dt);
    
    Vector3 drawPos = { hospital.position.x, 20.0f, hospital.position.z }; 
    DrawCube(drawPos, 80, 40, 60, WHITE);
    DrawCubeWires(drawPos, 80, 40, 60, LIGHTGRAY);
    
    DrawCube(drawPos, 81, 8, 61, RED);
    
    Vector3 crossPos = {hospital.position.x, 42.0f, hospital.position.z + 10.0f};
    DrawCube(crossPos, 20.0f, 4.0f, 4.0f, RED);
    DrawCube(crossPos, 4.0f, 4.0f, 20.0f, RED);
    
    Vector3 parkingPos = { hospital.position.x + 60.0f, 0.2f, hospital.position.z };
    DrawCubeWires(parkingPos, 50.0f, 0.0f, 20.0f, YELLOW);
    
    for (auto* ev : emergencyVehicles) {
        ev->draw();
    }
}

std::vector<EmergencyVehicle*>& EmergencyManager::getEmergencyVehicles() {
    return emergencyVehicles;
}

const Hospital& EmergencyManager::getHospital() const {
    return hospital;
}
