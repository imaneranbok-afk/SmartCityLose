#ifndef TRAFFICMANAGER_H
#define TRAFFICMANAGER_H

#include "Vehicule.h"
#include <vector>
#include <memory>
#include "../RoadNetwork.h"

class TrafficManager {
private:
    std::vector<std::unique_ptr<Vehicule>> vehicles;
    // Optional pointer to the road network for leader assignment
    RoadNetwork* network = nullptr;

public:
    void addVehicle(std::unique_ptr<Vehicule> v);
    void removeFinishedVehicles();
    void update(float deltaTime);
    void draw();
    // Renvoi une référence const pour la lecture
    const std::vector<std::unique_ptr<Vehicule>>& getVehicles() const { return vehicles; }
    // Reference mutable pour modification
    std::vector<std::unique_ptr<Vehicule>>& getVehiclesCheck() { return vehicles; }
    int getVehicleCount() const { return static_cast<int>(vehicles.size()); }
    const std::vector<Vector3> getVehiclePositions() const;

    void setRoadNetwork(RoadNetwork* net) { network = net; }
};

#endif
