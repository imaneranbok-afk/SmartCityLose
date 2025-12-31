#include "Vehicules/Car.h"

Car::Car(Vector3 startPos, CarModel model, int id)
    : Vehicule(startPos, 20.0f, 2.0f, Model{}, 1.0f, RED),
      modelType(model),
      state(CarState::IDLE),
      carId(id),
      fuelLevel(100.0f),
      fuelConsumption(0.12f) // Ajustez la consommation ici
{
    float maxSpd, accel;
    Color color;

    // On configure les constantes selon le modèle choisi
    configureModel(model, maxSpd, accel, color);

    // On met à jour les membres de la classe parente Vehicule
    this->maxSpeed = maxSpd;
    this->acceleration = accel;
    this->debugColor = color;

    // Chargement du modèle 3D (Raylib) si disponible
    // Note : si le fichier est introuvable, on conserve Model{} (meshCount == 0) et on dessinera un cube de debug
    const char* carPath = "resources/car.glb";
    if (FileExists(carPath)) {
        this->model = LoadModel(carPath);
    } else {
        TraceLog(LOG_WARNING, "[Car] Fichier de modèle introuvable : %s", carPath);
    }
    applyRotationFix();
}

// New overload: construct Car from an already-loaded Raylib Model
Car::Car(Vector3 startPos, Model m, int id)
    : Vehicule(startPos, 20.0f, 2.0f, m, 1.0f, RED),
      modelType(CarModel::DODGE_CHALLENGER),
      state(CarState::IDLE),
      carId(id),
      fuelLevel(100.0f),
      fuelConsumption(0.12f)
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

Car::~Car() {
    // UnloadModel(this->model); // Déchargement si nécessaire
}

void Car::update(float deltaTime) {
    // 1. Vérification du carburant
    if (fuelLevel <= 0.0f) {
        state = CarState::OUT_OF_FUEL;
        setWaiting(true); // Arrête le mouvement via Vehicule
        currentSpeed = 0.0f;
        return;
    }

    // 2. Mise à jour de la physique (héritée de Vehicule)
    updateMovement(deltaTime);

    // 3. Logique de consommation (DÉSACTIVÉE pour démo continue)
    // fuelLevel -= (currentSpeed / maxSpeed) * fuelConsumption * deltaTime;
    // if (fuelLevel < 0.0f) fuelLevel = 0.0f;

    // 4. Mise à jour de l'automate d'état
    if (hasReachedDestination()) {
        state = CarState::ARRIVED;
    } else if (currentSpeed > 0.1f) {
        state = CarState::MOVING;
    } else {
        state = CarState::IDLE;
    }
}

void Car::draw() {
    // Utilise la routine de rendu standard de Vehicule
    Vehicule::draw();
}

void Car::refuel(float amount) {
    fuelLevel += amount;
    if (fuelLevel > 100.0f) fuelLevel = 100.0f;
    
    // Si on a de nouveau du carburant, on sort de l'état de panne
    if (fuelLevel > 0.0f && state == CarState::OUT_OF_FUEL) {
        state = CarState::IDLE;
        setWaiting(false);
    }
}

void Car::configureModel(CarModel model, float& maxSpeed, float& accel, Color& color) {
    switch (model) {
        case CarModel::DODGE_CHALLENGER:
            maxSpeed = 30.0f; accel = 3.5f; color = RED; break;
        case CarModel::CHEVROLET_CAMARO:
            maxSpeed = 28.0f; accel = 3.2f; color = ORANGE; break;
        case CarModel::NISSAN_GTR:
            maxSpeed = 32.0f; accel = 4.0f; color = BLUE; break;
        case CarModel::SUV_MODEL:
            maxSpeed = 18.0f; accel = 1.8f; color = DARKGREEN; break;
        case CarModel::TAXI:
            maxSpeed = 22.0f; accel = 2.5f; color = YELLOW; break;
        case CarModel::TRUCK:
            maxSpeed = 15.0f; accel = 1.2f; color = DARKBLUE; break;
        case CarModel::BUS:
            maxSpeed = 16.0f; accel = 1.3f; color = SKYBLUE; break;
        case CarModel::FERRARI:
            maxSpeed = 40.0f; accel = 5.0f; color = RED; break;
        case CarModel::TESLA:
            maxSpeed = 35.0f; accel = 4.5f; color = BLACK; break;
        case CarModel::CONVERTIBLE:
            maxSpeed = 33.0f; accel = 4.0f; color = GOLD; break;
        case CarModel::CAR_BLANC:
            maxSpeed = 25.0f; accel = 2.8f; color = WHITE; break;
        case CarModel::GENERIC_RED:
            maxSpeed = 30.0f; accel = 3.5f; color = RED; break;
        case CarModel::GENERIC_CAR_3:
            maxSpeed = 28.0f; accel = 3.0f; color = BLUE; break;
        case CarModel::GENERIC_MODEL_1:
            maxSpeed = 26.0f; accel = 2.9f; color = GREEN; break;
        default:
            maxSpeed = 20.0f; accel = 2.0f; color = GRAY; break;
    }
}

// Final fix: User confirms Truck (0.0f) is straight. Bus needs to be same.
void Car::applyRotationFix() {
    switch (modelType) {
        case CarModel::BUS:
            rotationOffset = 180.0f; // 0.0f was "inclini" (maybe loose term for reverse or off?). 90 was horizontal. Trying 180.
            break;
        case CarModel::TRUCK:
            rotationOffset = 0.0f;
            break;
        default:
            rotationOffset = 0.0f; 
            break;
    }
}