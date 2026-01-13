#include "Vehicules/VehiculeFactory.h"
#include "Vehicules/ModelManager.h"
#include <map>

// Internal helpers (not exposed in header) to handle model selection and creation
namespace {
    std::unique_ptr<Vehicule> createVehicleFromCarModel(CarModel model, Vector3 startPos, int id = 0) {
        ModelManager& mm = ModelManager::getInstance();

            // Cars (default)
        std::string category = "CAR";
        Model m = mm.getRandomModel(category);
        if (m.meshCount > 0) {
            return std::make_unique<Car>(startPos, m, id);
        }

        return std::make_unique<Car>(startPos, model, id);
    }

    std::vector<std::unique_ptr<Vehicule>> createFleetFromCarModel(
        CarModel model, int count, Vector3 startPos, Vector3 spacing)
    {
        std::vector<std::unique_ptr<Vehicule>> fleet;

        for (int i = 0; i < count; i++) {
            Vector3 pos = {
                startPos.x + spacing.x * i,
                startPos.y,
                startPos.z + spacing.z * i
            };
            fleet.push_back(createVehicleFromCarModel(model, pos, i));
        }

        return fleet;
    }
}

std::unique_ptr<Vehicule> VehiculeFactory::createVehicule(VehiculeType type, Vector3 position) {
    switch (type) {
        case VehiculeType::CAR: {
            auto v = createVehicleFromCarModel(CarModel::CAR_BLANC, position);
            if (hasDefaultParams(type)) {
                auto p = getDefaultParams(type);
                v->setMaxSpeed(p.maxSpeed);
                v->setAcceleration(p.acceleration);
                v->setScale(p.length > 0.0f ? p.length : v->isLargeVehicle() ? 1.0f : 1.0f);
                v->setDebugColor(p.color);
            }
            return v;
        }
        case VehiculeType::BUS: {
            auto v = std::make_unique<Bus>(position);
            if (hasDefaultParams(type)) {
                auto p = getDefaultParams(type);
                v->setMaxSpeed(p.maxSpeed);
                v->setAcceleration(p.acceleration);
                v->setScale(p.length > 0.0f ? p.length : v->isLargeVehicle() ? 1.0f : 1.0f);
                v->setDebugColor(p.color);
            }
            return v;
        }
        case VehiculeType::TRUCK: {
            auto v = std::make_unique<Truck>(position);
            if (hasDefaultParams(type)) {
                auto p = getDefaultParams(type);
                v->setMaxSpeed(p.maxSpeed);
                v->setAcceleration(p.acceleration);
                v->setScale(p.length > 0.0f ? p.length : v->isLargeVehicle() ? 1.0f : 1.0f);
                v->setDebugColor(p.color);
            }
            return v;
        }
        default:
            return nullptr;
    }
}

std::unique_ptr<Vehicule> VehiculeFactory::createVehicule(VehiculeType type, Vector3 position, const std::string& modelPath) {
    switch (type) {
        case VehiculeType::CAR: {
            auto v = std::make_unique<Car>(position, modelPath);
            if (hasDefaultParams(type)) {
                auto p = getDefaultParams(type);
                v->setMaxSpeed(p.maxSpeed);
                v->setAcceleration(p.acceleration);
                v->setScale(p.length > 0.0f ? p.length : v->isLargeVehicle() ? 1.0f : 1.0f);
                v->setDebugColor(p.color);
            }
            return v;
        }
        case VehiculeType::BUS: {
            auto v = std::make_unique<Bus>(position, modelPath);
            if (hasDefaultParams(type)) {
                auto p = getDefaultParams(type);
                v->setMaxSpeed(p.maxSpeed);
                v->setAcceleration(p.acceleration);
                v->setScale(p.length > 0.0f ? p.length : v->isLargeVehicle() ? 1.0f : 1.0f);
                v->setDebugColor(p.color);
            }
            return v;
        }
        case VehiculeType::TRUCK: {
            auto v = std::make_unique<Truck>(position, modelPath);
            if (hasDefaultParams(type)) {
                auto p = getDefaultParams(type);
                v->setMaxSpeed(p.maxSpeed);
                v->setAcceleration(p.acceleration);
                v->setScale(p.length > 0.0f ? p.length : v->isLargeVehicle() ? 1.0f : 1.0f);
                v->setDebugColor(p.color);
            }
            return v;
        }
        default:
            return nullptr;
    }
    return nullptr;
}

// Storage for runtime default params (configured via MapLoader)
static std::map<VehiculeType, VehiculeFactory::VehicleParams> g_defaultParams;

void VehiculeFactory::setDefaultParams(VehiculeType type, const VehicleParams& params) {
    g_defaultParams[type] = params;
}

bool VehiculeFactory::hasDefaultParams(VehiculeType type) {
    return g_defaultParams.find(type) != g_defaultParams.end();
}

VehiculeFactory::VehicleParams VehiculeFactory::getDefaultParams(VehiculeType type) {
    auto it = g_defaultParams.find(type);
    if (it != g_defaultParams.end()) return it->second;
    return VehicleParams();
}