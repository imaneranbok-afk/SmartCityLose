#include "Vehicules/Vehicule.h"
#include "Vehicules/TrafficManager.h"
#include <cmath>
#include <algorithm>

Vehicule::Vehicule(Vector3 startPos, float maxSpd, float accel, Model mdl, float sc, Color dColor)
    : position(startPos),
      maxSpeed(maxSpd),
      acceleration(accel),
      model(mdl),
      scale(sc),
      debugColor(dColor),
      ownModel(false),
      angle(0.0f)
{
    currentSpeed = 0.0f;
    isFinished = false;
    isWaiting = false;
    state = State::ON_ROAD; // Default, will be set by setRoute
    leader = nullptr;
    updateYOffset();
}

// Load Model from file path and take ownership
Vehicule::Vehicule(Vector3 startPos, float maxSpd, float accel, const std::string& modelPath, float sc, Color dColor)
    : position(startPos),
      maxSpeed(maxSpd),
      acceleration(accel),
      scale(sc),
      debugColor(dColor),
      ownModel(false),
      angle(0.0f)
{
    currentSpeed = 0.0f;
    isFinished = false;
    isWaiting = false;
    state = State::ON_ROAD; // Default
    leader = nullptr;

    // Support spaces in file names; use std::string to hold path
    model = LoadModel(modelPath.c_str());
    ownModel = (model.meshCount > 0);
    updateYOffset();
}

Vehicule::~Vehicule() {
    if (ownModel && model.meshCount > 0) {
        UnloadModel(model);
    }
}

bool Vehicule::hasLoadedModel() const {
    return model.meshCount > 0;
}

// === NOUVELLE LOGIQUE PHYSIQUE ===

// Normalise un angle en radians entre [0, 2PI] (pour ronds-points anti-horaires simplifiés)
static float NormalizeAngleRad(float a) {
    while (a < 0) a += 2.0f * PI;
    while (a >= 2.0f * PI) a -= 2.0f * PI;
    return a;
}

// Différence d'angle courte signée (-PI à PI)
static float AngleDiff(float target, float current) {
    float diff = target - current;
    while (diff < -PI) diff += 2.0f * PI;
    while (diff > PI) diff -= 2.0f * PI;
    return diff;
}

void Vehicule::setRoute(const std::deque<RoadSegment*>& newRoute) {
    route = newRoute;
    if (!route.empty()) {
        currentRoad = route.front();
        route.pop_front();
        t_param = 0.0f;
        state = State::ON_ROAD;
        position = currentRoad->GetTrafficLanePosition(currentLane, 0.0f);
        Vector3 dir = currentRoad->GetDirection();
        angle = atan2f(dir.x, dir.z);
        isFinished = false;
    } else {
        isFinished = true;
    }
}

void Vehicule::setLane(int laneId) {
    currentLane = std::clamp(laneId, 0, 3);
}

void Vehicule::updatePhysics(float dt) {
    if (isFinished) return;
    
    // === SAFETY DISTANCE CHECK (Optimisée pour éviter les blocages) ===
    const float MINIMUM_SAFETY_DISTANCE = 22.0f;
    const float CRITICAL_SAFETY_DISTANCE = 10.0f;
    
    isWaiting = false;
    if (leader != nullptr) {
        Vector3 toLeader = Vector3Subtract(leader->getPosition(), position);
        float distToLeader = Vector3Length(toLeader);
        
        Vector3 dir = {sinf(angle), 0, cosf(angle)};
        float dot = Vector3DotProduct(Vector3Normalize(toLeader), dir);
        
        if (dot > 0.4f) { 
            if (distToLeader < CRITICAL_SAFETY_DISTANCE) {
                currentSpeed = 0.0f;
                isWaiting = true;
                // Ne pas return ici pour laisser la machine à état traiter l'orientation
            }
            else if (distToLeader < MINIMUM_SAFETY_DISTANCE) {
                currentSpeed -= acceleration * dt * 4.0f;
                if (currentSpeed < 10.0f) currentSpeed = 10.0f;
            }
        }
    }
    
    if (currentSpeed < maxSpeed) {
        currentSpeed += acceleration * dt;
        if (currentSpeed > maxSpeed) currentSpeed = maxSpeed;
    }

    // --- Machine à États ---
    bool stateProcessed = false;
    
    // ÉTAT : SUR ROUTE
    if (state == State::ON_ROAD) {
        stateProcessed = true;
        if (!currentRoad) return;

        // 1. Avancer paramètre t
        float len = currentRoad->GetLength();
        if (len < 0.1f) len = 0.1f;

        t_param += (currentSpeed * dt) / len;

        // 2. Calculer Position Exacte (Centrée sur voie)
        float t_clamped = std::min(t_param, 1.0f);
        Vector3 targetPos = currentRoad->GetTrafficLanePosition(currentLane, t_clamped);
        
        // Alignement strict avant l'entrée
        float blendFactor = (t_param > 0.85f ? 15.0f : 8.0f) * dt; 
        if (blendFactor > 1.0f) blendFactor = 1.0f;
        
        position.x = position.x + (targetPos.x - position.x) * blendFactor;
        position.z = position.z + (targetPos.z - position.z) * blendFactor;
        position.y = targetPos.y;

        Node* endNode = currentRoad->GetEndNode();
        Vector3 endPos = endNode->GetPosition();
        bool isNode5 = (fabs(endPos.x - 350.0f) < 10.0f && fabs(endPos.z) < 10.0f);
        // DÉTECTION LARGE : Type ROUNDABOUT ou Rayon important (>25.0f)
        bool headingToRoundabout = (endNode->GetType() == ROUNDABOUT || endNode->GetRadius() > 25.0f || isNode5);
        
        float limitT = 1.0f;
        // On suit la route jusqu'au raccordement exact pour éviter tout saut
        limitT = 1.0f; 

        // 3. Orientation
        if (t_param < limitT) {
            float t_lookahead = std::min(t_param + 0.05f, 1.0f);
            Vector3 nextP = currentRoad->GetTrafficLanePosition(currentLane, t_lookahead);
            Vector3 dir = Vector3Subtract(nextP, position);
            if (Vector3LengthSqr(dir) > 0.001f) {
                angle = atan2f(dir.x, dir.z);
            }
        }

        if (t_param >= limitT) {
            if (headingToRoundabout) {
                if (route.empty()) { isFinished = true; }
                else {
                    RoadSegment* nextRoad = route.front(); route.pop_front();

                    state = State::ENTER_ROUNDABOUT;
                    rabContext.active = true;
                    rabContext.center = endNode->GetPosition();
                    
                    float nodeRad = endNode->GetRadius();
                    // PRÉCISION GÉOMÉTRIQUE : Centres exacts des voies (1.0 -> 0.6 total)
                    if (currentLane == 0) rabContext.radius = nodeRad * 0.90f;
                    else rabContext.radius = nodeRad * 0.70f;
                    
                    rabContext.nextRoad = nextRoad;

                    // --- TRANSITION CONTINUE SANS SAUT ---
                    transContext.startPos = position; 
                    transContext.startAngle = angle;
                    
                    Vector3 toStart = Vector3Subtract(position, rabContext.center);
                    rabContext.currentAngle = atan2f(toStart.z, toStart.x);
    
                    Vector3 nextLaneStart = nextRoad->GetTrafficLanePosition(currentLane, 0.0f);
                    Vector3 toExit = Vector3Subtract(nextLaneStart, rabContext.center);
                    rabContext.exitAngle = atan2f(toExit.z, toExit.x);
                    
                    // Calcul de l'angle TOTAL restant à parcourir (Sens Anti-horaire strict)
                    float diff = rabContext.exitAngle - rabContext.currentAngle;
                    while (diff <= 0.0f) diff += 2.0f * PI; 
                    rabContext.angleRemaining = diff;
                    
                    transContext.progress = 0.0f;
                    transContext.duration = 0.5f; // Transition courte et précise
                }
            } else {
                if (route.empty()) { isFinished = true; return; }

                RoadSegment* nextRoad = route.front(); route.pop_front();

                // --- DISTRIBUTE TRAFFIC: Pick random lane on next road ---
                int fwdLanes = nextRoad->GetLanes() / 2;
                if (fwdLanes < 1) fwdLanes = 1;
                currentLane = GetRandomValue(0, fwdLanes - 1);

                transContext.startPos = position;
                transContext.endPos = nextRoad->GetTrafficLanePosition(currentLane, 0.0f);
                transContext.controlPos = endNode->GetPosition(); 
                transContext.startAngle = angle;
                
                Vector3 nextDir = nextRoad->GetDirection();
                transContext.endAngle = atan2f(nextDir.x, nextDir.z);
                
                transContext.progress = 0.0f;
                transContext.nextRoad = nextRoad;
                
                float dist1 = Vector3Distance(transContext.startPos, transContext.controlPos);
                float dist2 = Vector3Distance(transContext.controlPos, transContext.endPos);
                float totalDist = dist1 + dist2;
                float spd = std::max(currentSpeed, 10.0f);
                
                transContext.duration = totalDist / spd;
                transContext.duration = std::clamp(transContext.duration, 0.5f, 2.5f);
                state = State::INTERSECTION_TRANSITION;
            }
        }
    }

    // ÉTAT : ENTRÉE ROND-POINT
    if (state == State::ENTER_ROUNDABOUT) {
        stateProcessed = true;
        // ZÉRO SAUT : Transition par spirale radiale
        transContext.progress += dt / transContext.duration;
        float t = std::min(transContext.progress, 1.0f);
 
        // 1. Rayon actuel
        float startRad = Vector3Distance(transContext.startPos, rabContext.center);
        float currentRadius = startRad + (rabContext.radius - startRad) * t;
 
        // 2. Mise à jour de l'angle
        float angularSpeed = currentSpeed / std::max(currentRadius, 1.0f);
        float stepAngle = angularSpeed * dt;
        rabContext.currentAngle += stepAngle;
        rabContext.angleRemaining -= stepAngle;
        
        // 3. Position
        position.x = rabContext.center.x + currentRadius * cosf(rabContext.currentAngle);
        position.z = rabContext.center.z + currentRadius * sinf(rabContext.currentAngle);
 
        // 4. Orientation
        Vector3 tangent = { -sinf(rabContext.currentAngle), 0.0f, cosf(rabContext.currentAngle) };
        float targetAngle = atan2f(tangent.x, tangent.z);
        angle += AngleDiff(targetAngle, angle) * 8.0f * dt; 
 
        if (t >= 1.0f) {
            state = State::IN_ROUNDABOUT;
        }
    }

    // ÉTAT : DANS ROND-POINT
    if (state == State::IN_ROUNDABOUT) {
        stateProcessed = true;
        float angularSpeed = currentSpeed / std::max(rabContext.radius, 1.0f);
        float stepAngle = angularSpeed * dt;
        rabContext.currentAngle += stepAngle;
        rabContext.angleRemaining -= stepAngle;
        
        // Normalisation visuelle de l'angle
        while (rabContext.currentAngle > PI) rabContext.currentAngle -= 2.0f * PI;
        while (rabContext.currentAngle < -PI) rabContext.currentAngle += 2.0f * PI;
 
        // Positionnement au milieu de la voie
        position.x = rabContext.center.x + rabContext.radius * cosf(rabContext.currentAngle);
        position.z = rabContext.center.z + rabContext.radius * sinf(rabContext.currentAngle);
 
        // Orientation STRICTE vers la tangente (Précision maximale)
        Vector3 tangent = { -sinf(rabContext.currentAngle), 0.0f, cosf(rabContext.currentAngle) };
        angle = atan2f(tangent.x, tangent.z);
 
        // Sortie dès que l'angle à parcourir est épuisé
        if (rabContext.angleRemaining <= 0.0f) {
            state = State::EXIT_ROUNDABOUT;
        }
    }
    // ÉTAT : SORTIE ROND-POINT
    if (state == State::EXIT_ROUNDABOUT) {
        stateProcessed = true;
        // Switch to the next road and resume motion
        currentRoad = rabContext.nextRoad;
        
        // Respect de la logique des voies : On garde la même voie ou on l'adapte intelligemment
        // Suppression du GetRandomValue pour respecter la direction souhaitée
        if (currentRoad) {
             int fwdLanes = currentRoad->GetLanes() / 2;
             if (fwdLanes < 1) fwdLanes = 1;
             if (currentLane >= fwdLanes) currentLane = fwdLanes - 1;
             // Pas de randomisation ici pour garder la cohérence du trajet
        }
        
        rabContext.active = false;
        isWaiting = false;

        float roadLen = currentRoad ? currentRoad->GetLength() : 0.0f;
        if (roadLen > 1e-4f) {
            // 1. Calculate t_param based on actual position (Continuity of parameter)
            // This prevents longitudinal jumps (teleporting forward/back)
            float computedT = currentRoad->ComputeProgressOnSegment(position);
            
            if (computedT < 0.0f) {
                // Fallback: manual projection if ComputeProgress fails
                Vector3 startPos = currentRoad->GetStartNode()->GetPosition();
                Vector3 dir = currentRoad->GetDirection(); 
                Vector3 toVeh = Vector3Subtract(position, startPos);
                float distProj = Vector3DotProduct(toVeh, dir);
                computedT = distProj / roadLen;
            }
            t_param = std::clamp(computedT, 0.0f, 1.0f);

            // 2. Blend Position (Smoothing of visual pos)
            // Mirrors the logic in ENTER_ROUNDABOUT (blendFactor = 0.3f)
            // This prevents hard lateral snapping to the lane center
            Vector3 targetPos = currentRoad->GetTrafficLanePosition(currentLane, t_param);
            float blendFactor = 0.3f;
            position.x = position.x + (targetPos.x - position.x) * blendFactor;
            position.z = position.z + (targetPos.z - position.z) * blendFactor;

            // 3. Align Angle (AVEC LISSAGE)
            Vector3 roadDir = currentRoad->GetDirection();
            float targetRoadAngle = atan2f(roadDir.x, roadDir.z);
            angle += AngleDiff(targetRoadAngle, angle) * 10.0f * dt;
        } else {
            t_param = 0.0f;
        }

        state = State::ON_ROAD;
    }
    // ÉTAT : TRANSITION INTERSECTION BÉZIER
    if (state == State::INTERSECTION_TRANSITION) {
        stateProcessed = true;
        if (transContext.duration > 0.001f) {
            transContext.progress += dt / transContext.duration;
        } else {
            transContext.progress = 1.0f;
        }

        if (transContext.progress >= 1.0f) {
            // Transition Finished
            state = State::ON_ROAD;
            currentRoad = transContext.nextRoad;
            t_param = 0.0f;
            position = transContext.endPos; // Final snap to exact start of next road
            
            // Align angle smoothly to the new road's direction
            Vector3 roadDir = currentRoad->GetDirection();
            float targetRoadAngle = atan2f(roadDir.x, roadDir.z);
            angle += AngleDiff(targetRoadAngle, angle) * 10.0f * dt;
        } else {
            // Quadratic Bezier Calculation
            float t = transContext.progress;
            float invT = 1.0f - t;
            
            // P(t) = (1-t)^2 * P0 + 2(1-t)t * P1 + t^2 * P2
            Vector3 p0 = transContext.startPos;
            Vector3 p1 = transContext.controlPos;
            Vector3 p2 = transContext.endPos;
            
            Vector3 newPos;
            newPos.x = (invT * invT * p0.x) + (2 * invT * t * p1.x) + (t * t * p2.x);
            newPos.y = p0.y; // Keep Y constant
            newPos.z = (invT * invT * p0.z) + (2 * invT * t * p1.z) + (t * t * p2.z);
            
            // Calculate tangent for angle registration
            Vector3 tangent;
            tangent.x = 2.0f * invT * (p1.x - p0.x) + 2.0f * t * (p2.x - p1.x);
            tangent.z = 2.0f * invT * (p1.z - p0.z) + 2.0f * t * (p2.z - p1.z);

            if (Vector3LengthSqr(tangent) > 0.001f) {
                angle = atan2f(tangent.x, tangent.z);
            }
            
            position = newPos;
        }
    }
}

void Vehicule::draw() {
    if (model.meshCount == 0) {
        DrawCube(position, 2.0f, 2.0f, 4.0f, debugColor);
        DrawCubeWires(position, 2.1f, 2.1f, 4.1f, BLACK);
    } else {
        Vector3 renderPos = position;
        renderPos.y += 0.03f + yOffset; 
        DrawModelEx(model, renderPos, {0,1,0}, getRotationAngle(), {scale,scale,scale}, WHITE);
    }
}

void Vehicule::normalizeSize(float targetLength) {
    if (model.meshCount == 0) {
        scale = 2.5f; // Fallback for debug cube
        return;
    }

    BoundingBox box = GetModelBoundingBox(model);
    float w = box.max.x - box.min.x;
    float h = box.max.y - box.min.y;
    float l = box.max.z - box.min.z;

    float maxDim = std::max(w, std::max(h, l));

    if (maxDim < 0.01f) {
        scale = 2.5f;
    } else {
        scale = targetLength / maxDim;
    }

    if (scale < 0.5f) scale = 0.5f;

    updateYOffset();
}

bool Vehicule::IsApproachingDestination(Node* targetNode) const {
    if (!currentRoad || !targetNode) return false;
    // Route empty means currentRoad is the last segment
    return route.empty() && (currentRoad->GetEndNode() == targetNode);
}