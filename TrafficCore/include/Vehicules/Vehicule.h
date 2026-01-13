#ifndef VEHICULE_H
#define VEHICULE_H

#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <algorithm>
#include <string>
#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <algorithm>
#include <string>
#include <queue>
#include <deque> // For std::deque

class Vehicule {
protected:
    Vector3 position;
    float currentSpeed;
    float maxSpeed;
    float acceleration;
    
public:
    enum class State {
        ON_ROAD,
        ENTER_ROUNDABOUT,
        IN_ROUNDABOUT,
        EXIT_ROUNDABOUT,
        INTERSECTION_TRANSITION
    };

protected:

private:
    State state = State::ON_ROAD;
    
    // Navigation
    std::deque<class RoadSegment*> route; // Liste des routes à suivre
    class RoadSegment* currentRoad = nullptr;
    int currentLane = 0; // 0-3
    
    // Paramètres physiques
    float t_param = 0.0f; // Progression sur la route actuelle (0.0 à 1.0)
    
    // Paramètres Rond-point
    struct RoundaboutContext {
        Vector3 center;
        float radius;
        float currentAngle;     // Angle actuel dans le rond-point
        float exitAngle;        // Angle cible de sortie
        float angleRemaining;   // Angle total restant à parcourir
        int targetLane;         // Voie cible pour la sortie
        bool active = false;
        class RoadSegment* nextRoad = nullptr; // Route de sortie
    } rabContext;

    // Paramètres Transition Intersection (Courbe de Bézier quadratique)
    struct TransitionContext {
        Vector3 startPos;
        Vector3 controlPos; // Centre de l'intersection
        Vector3 endPos;
        float startAngle;
        float endAngle;
        float progress; // 0.0 à 1.0
        float duration; // Durée estimée en secondes
        class RoadSegment* nextRoad = nullptr;
    } transContext;

    

    // Visualisation / Debug
protected:
    Color debugColor;
    Model model;
    float scale;
    float yOffset = 0.0f;
    bool ownModel = false;
    bool isFinished = false;
    bool isWaiting = false;

    // Leader car-following
    Vehicule* leader = nullptr;

    // Physique & Orientation
    float angle = 0.0f; // Radians
    
public:
    Vehicule(Vector3 startPos, float maxSpd, float accel, Model mdl, float sc, Color dColor);
    Vehicule(Vector3 startPos, float maxSpd, float accel, const std::string& modelPath, float sc, Color dColor);
    
    virtual ~Vehicule();

    virtual void update(float deltaTime) = 0;
    
    // === NOUVELLE LOGIQUE PHYSIQUE ===
    void updatePhysics(float dt);
    
    // Configuration
    void setRoute(const std::deque<class RoadSegment*>& newRoute);
    void setLane(int laneId); // 0 or 1 usually
    
    // Helpers
    const Vector3& getPosition() const { return position; }
    float getCurrentSpeed() const { return currentSpeed; }
    float getRotationAngle() const { return angle * 57.2958f; } 
    bool isWaitingStatus() const { return isWaiting; }
    bool hasReachedDestination() const { return isFinished; }
    bool readyToRemove() const { return isFinished; }
    int getLane() const { return currentLane; }
    State getState() const { return state; }
    
    // Setters pour tuning
    void setMaxSpeed(float s) { maxSpeed = s; }
    void setAcceleration(float a) { acceleration = a; }
    void setDebugColor(Color c) { debugColor = c; }
    void setScale(float s) { 
        scale = s;
        updateYOffset();
    }
    void setWaiting(bool w) { isWaiting = w; }
    void setLeader(Vehicule* l) { leader = l; }
    Vehicule* getLeader() const { return leader; }
    class RoadSegment* getCurrentRoad() const { return currentRoad; }
    class RoadSegment* getNextRoad() const { return route.empty() ? nullptr : route.front(); }
    
    virtual void draw();
    virtual bool isLargeVehicle() const { return false; }
    bool hasLoadedModel() const;
    void normalizeSize(float targetLength);
    void updateYOffset() {
        if (model.meshCount > 0) {
            BoundingBox box = GetModelBoundingBox(model);
            yOffset = -box.min.y * scale;
        } else {
            yOffset = 0.0f;
        }
    }

    virtual float getMaxSpeed() const { return maxSpeed; }
    virtual float getAcceleration() const { return acceleration; }

    bool IsApproachingDestination(class Node* targetNode) const;
};

#endif
