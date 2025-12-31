#ifndef BUS_H
#define BUS_H
#include "Vehicule.h"

class Bus : public Vehicule {
private:
    float stopTimer;
    bool isAtStop;
public:
    Bus(Vector3 pos, Model m);
    void update(float deltaTime) override;
};
#endif