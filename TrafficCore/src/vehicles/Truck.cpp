#include "Vehicules/Truck.h"

Truck::Truck(Vector3 pos, Model m)
    : Vehicule(pos, 50.0f, 2.0f, m, 1.0f, RED), cargoCapacity(20.0f), currentLoad(0.0f)
{
    // Truck: visually slow and heavy
    this->scale = 1.0f;
    this->maxSpeed = 50.0f;     // units/s
    this->acceleration = 2.0f;  // responsiveness
}

Truck::Truck(Vector3 pos, const std::string& modelPath)
    : Vehicule(pos, 50.0f, 2.0f, modelPath, 1.0f, RED), cargoCapacity(20.0f), currentLoad(0.0f)
{
    this->scale = 1.0f;
    this->maxSpeed = 50.0f;     // units/s
    this->acceleration = 2.0f;  // responsiveness
}

void Truck::update(float deltaTime) {
    // Apply load factor effect via acceleration modification? For now simple physics
    // Si on veut garder l'effet de charge, on pourrait moduler deltaTime mais c'est tricheur.
    // Restons simple : updatePhysics direct.
    updatePhysics(deltaTime);
}

bool Truck::isLargeVehicle() const {
    return true;
} 