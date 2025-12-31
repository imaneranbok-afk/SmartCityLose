#include "Vehicules/CarFactory.h"
#include "Vehicules/ModelManager.h"

std::unique_ptr<Car> CarFactory::createCar(CarModel model, Vector3 startPos, int id) {
    ModelManager& mm = ModelManager::getInstance();
    
    std::string category = "CAR";
    if (model == CarModel::TRUCK) category = "TRUCK";
    else if (model == CarModel::BUS) category = "BUS";
    
    Model m = mm.getRandomModel(category);
    if (m.meshCount > 0) {
        return std::make_unique<Car>(startPos, m, id);
    }

    // Fallback to enum-based constructor if no model loaded
    return std::make_unique<Car>(startPos, model, id);
}

std::vector<std::unique_ptr<Car>> CarFactory::createFleet(
    CarModel model, int count, Vector3 startPos, Vector3 spacing)
{
    std::vector<std::unique_ptr<Car>> fleet;

    ModelManager& mm = ModelManager::getInstance();

    for (int i = 0; i < count; i++) {
        Vector3 pos = {
            startPos.x + spacing.x * i,
            startPos.y,
            startPos.z + spacing.z * i
        };

        Model m = mm.getRandomModel("CAR");
        if (m.meshCount > 0) {
            fleet.push_back(std::make_unique<Car>(pos, m, i));
        } else {
            fleet.push_back(std::make_unique<Car>(pos, model, i));
        }
    }

    return fleet;
}
