#ifndef TRUCK_H
#define TRUCK_H

#include "Vehicule.h"
#include <string> // used for model path strings (supports spaces)

class Truck : public Vehicule {
private:
    float cargoCapacity; // Capacité de chargement en tonnes
    float currentLoad;   // Chargement actuel

public:
    // Constructeur avec modèle chargé
    Truck(Vector3 startPos, Model m);
    // Convenience constructor from file path (supports spaces)
    Truck(Vector3 startPos, const std::string& modelPath = "assets/models/truck.glb");

    virtual ~Truck() = default;

    void update(float deltaTime) override;

    bool isLargeVehicle() const override;

    float getMaxSpeed() const override { return maxSpeed; }
    float getAcceleration() const override { return acceleration; }

    // Méthodes spécifiques
    void loadCargo(float amount) {
        currentLoad += amount;
        if (currentLoad > cargoCapacity) currentLoad = cargoCapacity;
    }

    void unloadCargo() {
        currentLoad = 0.0f;
    }
};

#endif // TRUCK_H