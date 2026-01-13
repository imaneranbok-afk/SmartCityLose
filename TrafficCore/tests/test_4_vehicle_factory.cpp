#include <iostream>
#include <cassert>
#include "Vehicules/VehiculeFactory.h"
#include "Vehicules/Vehicule.h"

void test_vehicle_creation() {
    Vector3 pos = {0, 0, 0};
    
    auto car = VehiculeFactory::createVehicule(VehiculeType::CAR, pos);
    assert(car != nullptr);
    assert(car->getPosition().x == pos.x);
    
    auto bus = VehiculeFactory::createVehicule(VehiculeType::BUS, pos);
    assert(bus != nullptr);
    
    auto truck = VehiculeFactory::createVehicule(VehiculeType::TRUCK, pos);
    assert(truck != nullptr);

    // Additional quick checks for the new interface
    auto b2 = std::make_unique<Bus>(pos); // default loads assets/models/bus bleu.glb
    assert(b2 != nullptr && b2->isLargeVehicle());

    auto t2 = std::make_unique<Truck>(pos); // default loads assets/models/truck.glb
    assert(t2 != nullptr && t2->isLargeVehicle());

    auto c2 = std::make_unique<Car>(pos, "assets/models/Taxi (1).glb");
    assert(c2 != nullptr);
    
    std::cout << "Vehicle creation tests passed!" << std::endl;
}

int main() {
    std::cout << "Running VehiculeFactory tests..." << std::endl;
    test_vehicle_creation();
    std::cout << "All VehiculeFactory tests passed!" << std::endl;
    return 0;
}
