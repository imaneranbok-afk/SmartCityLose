#ifndef VEHICULE_H
#define VEHICULE_H

#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <algorithm>

class Vehicule {
protected:
    Vector3 position;
    float rotationAngle;
    float rotationOffset; // New: to correct model orientation
    float scale;
    Model model;
    Color debugColor;

    std::vector<Vector3> itinerary;
    int currentTargetIndex;

    float currentSpeed;
    float maxSpeed;
    float acceleration;

    bool isFinished;
    bool isWaiting;

    // Traffic state machine
    enum class State {
        DRIVING,
        FOLLOWING,
        STOPPED,
        YIELDING,
        WAITING_AT_INTERSECTION,
        EMERGENCY_BRAKE,
        PARKED
    } state;

    // Leader for car-following
    Vehicule* leader;
    float desiredSpacing; // desired gap in meters
    float minSpacing;     // minimum gap
    float timeHeadway;    // desired time headway (s)
    // IDM parameters
    float idmAccel;   // maximum acceleration (m/s^2)
    float idmDecel;   // comfortable deceleration (m/s^2)
    float idmDelta;   // acceleration exponent

public:
    Vehicule(Vector3 startPos, float maxSpd, float accel, Model mdl, float sc, Color dColor);
    virtual ~Vehicule();

    virtual void update(float deltaTime) = 0;
    virtual void draw();

    void updateMovement(float dt);
    void setItinerary(const std::vector<Vector3>& newPath);

    // Traffic helpers
    void updateState(float dt);
    void followLeader(float dt);
    void setLeader(Vehicule* l) { leader = l; }
    Vehicule* getLeader() const { return leader; }

    void setScale(float s) { scale = s; }
    void setRotationOffset(float deg) { rotationOffset = deg; }
    void normalizeSize(float targetLength);

    Vector3 getPosition() const { return position; }
    float getCurrentSpeed() const { return currentSpeed; }
    float getRotationAngle() const { return rotationAngle; }
    bool hasReachedDestination() const { return isFinished; }
    void setWaiting(bool w) { isWaiting = w; }
};

#endif
