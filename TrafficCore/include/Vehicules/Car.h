#ifndef CAR_H
#define CAR_H

#include "Vehicule.h"
#include <string> // used for model path strings (supports spaces)

// Enumération pour les différents types de modèles de voitures
enum class CarModel {
    TAXI,
    CONVERTIBLE,
    CAR_BLANC,
    GENERIC_MODEL_1
};

class Car : public Vehicule {
private:
    int carId;
    CarModel modelType;

    // Si la classe Vehicule ne stocke pas déjà le modèle Raylib
    // Model model; 

public:
    // Constructeur fusionné : prend la position de départ, le modèle spécifique, et l'ID
    // Note : On peut passer l'objet 'Model' de Raylib si nécessaire
    Car(Vector3 startPos, CarModel model, int id = 0);
    // Overload: construct from an already-loaded Raylib Model
    Car(Vector3 startPos, Model m, int id = 0);
    // Convenience constructor that loads a given .glb file (supports spaces in file names)
    Car(Vector3 startPos, const std::string& modelPath, CarModel model = CarModel::CAR_BLANC, int id = 0);
    
    virtual ~Car();

    // Méthodes de mise à jour et d'affichage (override de Vehicule)
    void update(float deltaTime) override;
    void draw() override;

    // Accesseurs
    int getCarId() const { return carId; }

    // Méthode statique pour configurer les performances selon le modèle choisi
    static void configureModel(CarModel model, float& maxSpeed, float& accel, Color& color);
    void applyRotationFix();

    bool isLargeVehicle() const override { return modelType == CarModel::GENERIC_MODEL_1; }

    // Performance accessors (polymorphic override)
    float getMaxSpeed() const override { return maxSpeed; }
    float getAcceleration() const override { return acceleration; }
};

#endif // CAR_H