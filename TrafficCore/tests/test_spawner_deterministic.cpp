#include <iostream>
#include <cassert>
#include "Vehicules/TrafficManager.h"

int main() {
    TrafficManager tm;
    // Single entry point
    tm.addEntryPoint("Main", {0.0f,0.0f,0.0f});

    // Schedule exact total of 5 vehicles
    tm.scheduleRoundRobinVehicles(5);

    auto modelResolver = [](VehiculeType t)->std::string { return std::string(); };
    auto itineraryResolver = [](const Vector3& pos)->std::vector<Vector3> { return std::vector<Vector3>(); };

    // Spawn them one by one
    for (int i = 0; i < 5; ++i) {
        bool ok = tm.spawnNext(modelResolver, itineraryResolver);
        assert(ok);
    }

    // Now queue should be empty, spawnNext must return false
    bool ok = tm.spawnNext(modelResolver, itineraryResolver);
    assert(!ok);

    // Vehicle count should be exactly 5
    assert(tm.getVehicleCount() == 5);

    std::cout << "Spawner deterministic test passed." << std::endl;
    return 0;
}
