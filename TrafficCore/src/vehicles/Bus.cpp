#include "Vehicules/Bus.h"

Bus::Bus(Vector3 pos, Model m) 
    : Vehicule(pos, 70.0f, 3.0f, m, 1.5f, GREEN), passengerCapacity(50), currentPassengers(0)
{
    // Bus: visually slower than car, tuned for readability
    this->scale = 1.0f;
    this->maxSpeed = 70.0f;    // units/s
    this->acceleration = 3.0f; // responsiveness
}

Bus::Bus(Vector3 pos, const std::string& modelPath)
    : Vehicule(pos, 70.0f, 3.0f, modelPath, 1.0f, BLUE), passengerCapacity(50), currentPassengers(0)
{
    this->scale = 1.0f;
    this->maxSpeed = 70.0f;    // units/s
    this->acceleration = 3.0f; // responsiveness
}

void Bus::update(float deltaTime) {
    // Bus no longer stops periodically; behave like other vehicles
    updatePhysics(deltaTime);
}