#include "Vehicules/Bus.h"

Bus::Bus(Vector3 pos, Model m) 
    : Vehicule(pos, 18.0f, 1.2f, m, 1.5f, GREEN), stopTimer(0), isAtStop(false) {}

void Bus::update(float deltaTime) {
    if (!hasReachedDestination()) {
        stopTimer += deltaTime;
        // Toutes le 12 secondes, le bus s'arrête pendant 4 secondes
        if (!isAtStop && stopTimer > 12.0f) {
            isAtStop = true;
            stopTimer = 0;
            setWaiting(true);
        }
        if (isAtStop && stopTimer > 4.0f) {
            isAtStop = false;
            stopTimer = 0;
            setWaiting(false);
        }
    }
    updateMovement(deltaTime);
}