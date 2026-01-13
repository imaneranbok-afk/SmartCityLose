#ifndef BUS_H
#define BUS_H

#include "Vehicule.h"
#include <string> // used for model path strings (supports spaces)

class Bus : public Vehicule {
private:
    int passengerCapacity;
    int currentPassengers;

public:
    Bus(Vector3 startPos, Model m);
    // Convenience constructor that loads from an asset (.glb). Supports spaces in filenames.
    Bus(Vector3 startPos, const std::string& modelPath = "assets/models/bus bleu.glb");

    virtual ~Bus() = default;

    void update(float deltaTime) override;

    bool isLargeVehicle() const override { return true; }

    float getMaxSpeed() const override { return maxSpeed; }
    float getAcceleration() const override { return acceleration; }

    void boardPassengers(int count) {
        currentPassengers += count;
        if (currentPassengers > passengerCapacity) currentPassengers = passengerCapacity;
    }

    void alightPassengers(int count) {
        currentPassengers -= count;
        if (currentPassengers < 0) currentPassengers = 0;
    }
    
    int getPassengerCount() const { return currentPassengers; }
};

#endif // BUS_H