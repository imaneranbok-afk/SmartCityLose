#include <iostream>
#include <cassert>
#include "Vehicules/VehiculeFactory.h"
#include "Vehicules/Vehicule.h"

void test_vehicle_creation() {
    Vector3 pos = {0, 0, 0};
    
    Vehicule* car = VehiculeFactory::createVehicule(VehiculeType::CAR, pos);
    assert(car != nullptr);
    assert(car->getPosition().x == pos.x);
    
    Vehicule* bus = VehiculeFactory::createVehicule(VehiculeType::BUS, pos);
    assert(bus != nullptr);
    
    Vehicule* truck = VehiculeFactory::createVehicule(VehiculeType::TRUCK, pos);
    assert(truck != nullptr);
    
    delete car;
    delete bus;
    delete truck;
    
    std::cout << "Vehicle creation tests passed!" << std::endl;
}

int main() {
    std::cout << "Running VehiculeFactory tests..." << std::endl;
    test_vehicle_creation();
    std::cout << "All VehiculeFactory tests passed!" << std::endl;
    return 0;
}
