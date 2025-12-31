#ifndef VEHICLE_FACTORY_H
#define VEHICLE_FACTORY_H

#include "Vehicule.h"
#include "raylib.h"

enum class VehiculeType { CAR, BUS, TRUCK };

class VehiculeFactory {
public:
    static Vehicule* createVehicule(VehiculeType type, Vector3 startPosition);
};

#endif