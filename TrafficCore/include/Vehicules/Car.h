#ifndef CAR_H
#define CAR_H

#include "Vehicule.h"
#include <string>

// Enumération pour les différents types de modèles de voitures
enum class CarModel {
    DODGE_CHALLENGER,
    CHEVROLET_CAMARO,
    NISSAN_GTR,
    SUV_MODEL,
    TAXI,
    TRUCK,
    BUS,
    FERRARI,
    CONVERTIBLE,
    TESLA,
    CAR_BLANC,
    GENERIC_RED,
    GENERIC_CAR_3,
    GENERIC_MODEL_1
};

// Enumération pour gérer l'automate d'état de la voiture
enum class CarState {
    IDLE,
    MOVING,
    ARRIVED,
    OUT_OF_FUEL
};

class Car : public Vehicule {
private:
    int carId;
    CarModel modelType;
    CarState state;
    
    float fuelLevel;
    float fuelConsumption;
    
    // Si la classe Vehicule ne stocke pas déjà le modèle Raylib
    // Model model; 

public:
    // Constructeur fusionné : prend la position de départ, le modèle spécifique, et l'ID
    // Note : On peut passer l'objet 'Model' de Raylib si nécessaire
    Car(Vector3 startPos, CarModel model, int id = 0);
    // Overload: construct from an already-loaded Raylib Model
    Car(Vector3 startPos, Model m, int id = 0);
    
    virtual ~Car();

    // Méthodes de mise à jour et d'affichage (override de Vehicule)
    void update(float deltaTime) override;
    void draw() override;

    // Gestion de l'essence
    void refuel(float amount);
    float getFuelLevel() const { return fuelLevel; }

    // Accesseurs
    int getCarId() const { return carId; }
    CarState getState() const { return state; }
    CarModel getModelType() const { return modelType; }

    // Méthode statique pour configurer les performances selon le modèle choisi
    static void configureModel(CarModel model, float& maxSpeed, float& accel, Color& color);
    void applyRotationFix();
};

#endif // CAR_H