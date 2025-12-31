#ifndef TRUCK_H
#define TRUCK_H
#include "Vehicule.h"

class Truck : public Vehicule {
public:
    // Vitesse max: 12.0, Accélération: 0.4 (très lent à démarrer)
    Truck(Vector3 pos, Model m);
    void update(float deltaTime) override;
};
#endif