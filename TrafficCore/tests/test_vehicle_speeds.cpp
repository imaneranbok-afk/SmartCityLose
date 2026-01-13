#include <iostream>
#include <cassert>
#include <cmath>
#include "Vehicules/Car.h"
#include "Vehicules/Bus.h"
#include "Vehicules/Truck.h"

static bool almost_equal(float a, float b, float eps = 1e-3f) {
    return fabsf(a - b) <= eps;
}

int main() {
    Vector3 pos = {0,0,0};

    Car c(pos, CarModel::GENERIC_MODEL_1, 1);
    Bus b(pos);
    Truck t(pos);

    // Check documented realistic speeds
    assert(almost_equal(c.getMaxSpeed(), 13.9f));
    assert(almost_equal(c.getAcceleration(), 2.5f));

    assert(almost_equal(b.getMaxSpeed(), 11.1f));
    assert(almost_equal(b.getAcceleration(), 1.5f));

    assert(almost_equal(t.getMaxSpeed(), 8.3f));
    assert(almost_equal(t.getAcceleration(), 1.0f));

    std::cout << "Vehicle speed constants test passed." << std::endl;
    return 0;
}
