#ifndef VEHICLE_FACTORY_H
#define VEHICLE_FACTORY_H

#include "Vehicule.h"
#include "Car.h"
#include "Bus.h"
#include "Truck.h"
#include "raylib.h"
#include <memory>
#include <vector>

enum class VehiculeType { CAR, BUS, TRUCK };

class VehiculeFactory {
public:
    struct VehicleParams {
        float maxSpeed = 0.0f;
        float acceleration = 0.0f;
        float length = 0.0f;
        Color color = WHITE;
    };

    // Configure defaults for a vehicle type (used by MapLoader)
    static void setDefaultParams(VehiculeType type, const VehicleParams& params);
    static bool hasDefaultParams(VehiculeType type);
    static VehicleParams getDefaultParams(VehiculeType type);

    // Generic factory returning owning pointer
    // - Use this for quick creation by type (CAR, BUS, TRUCK).
    // - Returns an owning `std::unique_ptr<Vehicule>` ready to be added to `TrafficManager`.
    // - For CAR, a sensible default CarModel is used unless you call `createCar` explicitly.
    static std::unique_ptr<Vehicule> createVehicule(VehiculeType type, Vector3 startPosition);

    // Overload that creates the vehicle using a specific .glb model path (supports spaces)
    // - `modelPath` should point to a valid .glb file (absolute or relative). Example:
    //   "../TrafficCore/assets/models/Taxi (1).glb"
    static std::unique_ptr<Vehicule> createVehicule(VehiculeType type, Vector3 startPosition, const std::string& modelPath);

    // Note: simplified public interface — only generic `createVehicule` (CAR/BUS/TRUCK).
    // Internal helpers exist in the implementation file; if you need groups or specific
    // model variants, modify the factory implementation accordingly.
};

#endif