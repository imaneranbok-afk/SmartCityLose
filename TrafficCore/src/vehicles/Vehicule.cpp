#include "Vehicules/Vehicule.h"

Vehicule::Vehicule(Vector3 startPos, float maxSpd, float accel, Model mdl, float sc, Color dColor)
    : position(startPos),
      maxSpeed(maxSpd),
      acceleration(accel),
      model(mdl),
      scale(sc),
      debugColor(dColor)
{
    rotationAngle = 0.0f;
    currentSpeed = 0.0f;
    currentTargetIndex = 0;
    isFinished = false;
    isWaiting = false;
    // traffic defaults
    state = State::DRIVING;
    leader = nullptr;
    desiredSpacing = 15.0f; // Increased from 6.0f for safer following
    minSpacing = 10.0f;     // Increased from 2.5f to accommodate trucks/buses
    timeHeadway = 1.8f;    // Increased from 1.5f for smoother braking
    // IDM defaults
    idmAccel = 1.5f;
    idmDecel = 4.0f;       // Increased braking power from 2.0f
    idmDelta = 4.0f;
    rotationOffset = 0.0f;
    groundingOffset = 0.0f;
}

Vehicule::~Vehicule() {
    // if (model.meshCount > 0) {
    //     UnloadModel(model); // FIX: Do NOT unload shared models managed by ModelManager
    // }
}

void Vehicule::setItinerary(const std::vector<Vector3>& newPath) {
    itinerary = newPath;
    currentTargetIndex = 0;
    isFinished = false;
    if (!itinerary.empty()) {
        position = itinerary[0];
    }
}

void Vehicule::updateMovement(float dt) {
    if (itinerary.empty() || isFinished) return;

    // Update state machine and leader following
    updateState(dt);

    if (isWaiting || state == State::STOPPED || state == State::PARKED) {
        currentSpeed = Lerp(currentSpeed, 0.0f, dt * 5.0f);
        if (currentSpeed < 0.1f) currentSpeed = 0.0f;
    } else if (leader != nullptr && state == State::FOLLOWING) {
        followLeader(dt);
    } else {
        if (currentSpeed < maxSpeed * 0.1f) currentSpeed = Lerp(currentSpeed, maxSpeed * 0.1f, dt);
        currentSpeed = Lerp(currentSpeed, maxSpeed, dt * acceleration);
    }
    
    // Prevent full stop unless WAITING
    if (state == State::DRIVING && currentSpeed < 0.5f) currentSpeed = 0.5f;

    if (currentSpeed <= 0.0f) return;

    Vector3 target = itinerary[currentTargetIndex];
    Vector3 direction = Vector3Subtract(target, position);
    float distance = Vector3Length(direction);

    if (distance < 1.0f) { // Increased from 0.5f
        currentTargetIndex++;
        if (currentTargetIndex >= itinerary.size()) {
            isFinished = true;
            currentSpeed = 0.0f;
        }
        return;
    }

    float targetAngle = atan2f(direction.x, direction.z) * RAD2DEG;
    rotationAngle = targetAngle;

    Vector3 velocity = Vector3Scale(Vector3Normalize(direction), currentSpeed * dt);
    position = Vector3Add(position, velocity);
}

void Vehicule::updateState(float dt) {
    if (isWaiting) {
        state = State::WAITING_AT_INTERSECTION;
        return;
    }

    if (leader != nullptr) {
        float dist = Vector3Distance(position, leader->getPosition());
        if (dist < desiredSpacing * 1.5f) {
            state = State::FOLLOWING;
            return;
        }
    }

    state = State::DRIVING;
}

void Vehicule::followLeader(float dt) {
    if (leader == nullptr) return;

    float dist = Vector3Distance(position, leader->getPosition());
    float v = currentSpeed;
    float v0 = std::max(0.1f, maxSpeed);
    float s0 = minSpacing;
    float T = timeHeadway;
    float deltaV = v - leader->getCurrentSpeed(); // closing speed (positive if we are faster)

    // safe handle for very small distances
    float s = std::max(0.1f, dist);

    // desired gap s* = s0 + v*T + v*deltaV / (2*sqrt(a*b))
    float sqrtTerm = 2.0f * sqrtf(idmAccel * idmDecel);
    float sStar = s0 + v * T + (v * deltaV) / std::max(0.01f, sqrtTerm);

    // IDM acceleration
    float accel = idmAccel * (1.0f - powf(v / v0, idmDelta) - (sStar / s) * (sStar / s));

    // integrate speed
    currentSpeed += accel * dt;
    if (currentSpeed < 0.0f) currentSpeed = 0.0f;
    if (currentSpeed > maxSpeed) currentSpeed = maxSpeed;
}

void Vehicule::draw() {
    if (model.meshCount == 0) {
        DrawCube(position, 2.0f, 2.0f, 4.0f, debugColor);
        DrawCubeWires(position, 2.1f, 2.1f, 4.1f, BLACK);
    } else {
        Vector3 renderPos = position;
        renderPos.y += 0.02f + groundingOffset; // Lift above lowered road surface (0.02f)
        DrawModelEx(model, renderPos, {0,1,0}, rotationAngle + rotationOffset, {scale,scale,scale}, WHITE);
    }
}

void Vehicule::normalizeSize(float targetLength) {
    if (model.meshCount == 0) {
        scale = 2.5f; // Falback for debug cube
        return;
    }
    
    BoundingBox box = GetModelBoundingBox(model);
    float w = box.max.x - box.min.x;
    float h = box.max.y - box.min.y;
    float l = box.max.z - box.min.z;
    
    // Use max dimension to ensure it fits in target size (or just length for alignment)
    float maxDim = std::max(w, std::max(h, l));
    
    if (maxDim < 0.01f) {
        scale = 2.5f;
    } else {
        scale = targetLength / maxDim;
    }

    // Dynamic grounding calculation
    groundingOffset = -box.min.y * scale;
    
    // Minimum scale clamp to avoid invisibility
    if (scale < 0.5f) scale = 0.5f;
    
    // If it's a truck or bus, maybe make it slightly bigger? 
    // But normalizeSize is intended to make them uniform. 
    // Let's rely on the targetLength passed in.
}
