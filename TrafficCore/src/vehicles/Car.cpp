#include "Vehicules/Car.h"
#include <string>

// Map CarModel to an actual .glb asset path. Extendable if you add more car models.
static std::string CarModelToPath(CarModel m) {
    switch (m) {
        case CarModel::TAXI: return "assets/models/Taxi (1).glb";
        case CarModel::CAR_BLANC: return "assets/models/carblanc.glb";
        case CarModel::CONVERTIBLE: return "assets/models/Convertible.glb";
        case CarModel::GENERIC_MODEL_1: return "assets/models/model (1).glb";
        default: return "assets/models/carblanc.glb";
    }
}

Car::Car(Vector3 startPos, CarModel model, int id)
    : Vehicule(startPos, 100.0f, 6.0f, CarModelToPath(model), 1.0f, RED),
      modelType(model),
      carId(id)
{
    float maxSpd, accel;
    Color color;

    // On configure les constantes selon le modèle choisi (valeurs en unités Raylib/sec)
    // Car: ~100 units/s (visual speed tuned for Raylib window)
    configureModel(model, maxSpd, accel, color);

    // On met à jour les membres de la classe parente Vehicule
    this->maxSpeed = maxSpd;          // units / s (Raylib scene units)
    this->acceleration = accel;       // responsiveness factor used with deltaTime
    this->debugColor = color;

    applyRotationFix();
}

// New overload: construct Car from an already-loaded Raylib Model
Car::Car(Vector3 startPos, Model m, int id)
    : Vehicule(startPos, 13.9f, 2.5f, m, 1.0f, RED),
      modelType(CarModel::CAR_BLANC),
      carId(id)
{
    float maxSpd, accel;
    Color color;
    // Use default modelType performance profile
    configureModel(modelType, maxSpd, accel, color);
    this->maxSpeed = maxSpd;
    this->acceleration = accel;
    this->debugColor = color;
    applyRotationFix();
}

Car::Car(Vector3 startPos, const std::string& modelPath, CarModel model, int id)
    : Vehicule(startPos, 13.9f, 2.5f, modelPath, 1.0f, RED),
      modelType(model),
      carId(id)
{
    float maxSpd, accel;
    Color color;
    configureModel(model, maxSpd, accel, color);
    this->maxSpeed = maxSpd;
    this->acceleration = accel;
    this->debugColor = color;
    applyRotationFix();
}

Car::~Car() {
    // UnloadModel(this->model); // Déchargement si nécessaire
}

void Car::update(float deltaTime) {
    // Update physics (inherited)
    updatePhysics(deltaTime);

    // State tracking removed (unused); keep logic minimal
}

void Car::draw() {
    // Utilise la routine de rendu standard de Vehicule
    Vehicule::draw();
}



void Car::configureModel(CarModel model, float& maxSpeed, float& accel, Color& color) {
    // Values tuned for Raylib visualization (units: Raylib units / second)
    // Chosen as visually readable & smooth: Car ~100 units/s
    (void)model; // keep API for future per-model tuning
    maxSpeed = 100.0f; // visual speed, units/s
    accel = 6.0f;      // responsiveness factor, higher => faster approach to vmax
    color = WHITE;
}

// Rotation offset removed; orientation is computed from heading now.
void Car::applyRotationFix() {
    // no-op: legacy rotationOffset removed
}