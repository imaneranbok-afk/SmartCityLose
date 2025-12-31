#ifndef CARFACTORY_H
#define CARFACTORY_H

#include "Car.h"
#include <memory>
#include <vector>

class CarFactory {
public:
    static std::unique_ptr<Car> createCar(CarModel model, Vector3 startPos, int id = 0);
    static std::vector<std::unique_ptr<Car>> createFleet(
        CarModel model, int count, Vector3 startPos, Vector3 spacing);
};

#endif
