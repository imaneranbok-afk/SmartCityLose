// demos/boukhalf_simulation.cpp
#include <utility>
#include "raylib.h"
#include "raymath.h"
#include "Vehicules/CarFactory.h"
#include "Vehicules/TrafficManager.h"
#include "Vehicules/Car.h"
#include "RoadNetwork.h"

#include <memory>
#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <cmath>
#include <filesystem>
#include "Vehicules/ModelManager.h"

// ==================== CAMERA TYPES ====================
enum CameraControlMode {  // ← ÉTAIT "CameraMode"
    CAM_ORBITAL,
    CAM_FREE_FLY,
    CAM_FOLLOW_VEHICLE
};

// ==================== CAMERA STATE ====================
struct CameraState {
    CameraControlMode mode;  // ← Type changé
    Vector2 angles;
    float distance;
    Vector3 focusPoint;
    float yaw, pitch;
    int followedVehicleId;
    bool smoothTransition;
    bool cinematicMode; // Nouvelle propriété
};

// ==================== GLOBALS ====================
Camera3D g_camera;
CameraState g_camState;

// ==================== PROTOTYPES ====================
void InitCamera();
void UpdateCamera(TrafficManager& trafficMgr, float dt);
void UpdateCameraOrbital(float dt);
void UpdateCameraFreeFly(float dt);
void UpdateCameraFollow(TrafficManager& trafficMgr, float dt);
void DrawEnvironment();
void DrawUIPanel(int x, int y, int width, int height, const char* title);
void DrawCameraInfo();

CarModel StringToCarModel(const std::string& name);
void CreateTestNetwork(RoadNetwork& network);

// ==================== HELPER: PATH GENERATION ====================
std::vector<Vector3> GeneratePathPoints(const RoadNetwork& network, const std::vector<Node*>& nodePath, int samplesPerSegment) {
    std::vector<Vector3> points;
    if (nodePath.size() < 2) return points;

    for (size_t i = 0; i < nodePath.size() - 1; ++i) {
        Node* startNode = nodePath[i];
        Node* endNode = nodePath[i+1];
        
        // Find connecting road
        RoadSegment* connecting = nullptr;
        for (const auto& segPtr : network.GetRoadSegments()) {
            RoadSegment* s = segPtr.get();
            if (!s) continue;
            if ((s->GetStartNode() == startNode && s->GetEndNode() == endNode) ||
                (s->GetStartNode() == endNode && s->GetEndNode() == startNode)) {
                connecting = s;
                break;
            }
        }
        
        if (!connecting) {
            // Fallback: direct line
            points.push_back(startNode->GetPosition());
            continue;
        }

        // Determine lane
        // Determine lane logic for RHT (Right Hand Traffic)
        int lanes = connecting->GetLanes();
        int laneIdx = 0;
        bool isReverse = (connecting->GetEndNode() == startNode);

        if (lanes >= 2) {
             if (!isReverse) {
                 // Forward -> Use Right side (High indices)
                 // e.g. 4 lanes: 2, 3
                 int minLane = lanes / 2;
                 int maxLane = lanes - 1;
                 laneIdx = GetRandomValue(minLane, maxLane);
             } else {
                 // Reverse -> Use Left side (Low indices)
                 // e.g. 4 lanes: 0, 1
                 // Note: Left side of segment is Right side for Reverse Driver.
                 int minLane = 0;
                 int maxLane = (lanes / 2) - 1;
                 laneIdx = GetRandomValue(minLane, maxLane);
             }
        } else {
             laneIdx = 0;
        }

        // --- ROUNDABOUT ENTRY/EXIT LOGIC ---
        // If we are entering a roundabout (endNode is ROUNDABOUT), we just go to the edge.
        // If we are traversing a roundabout (startNode is ROUNDABOUT, endNode is ROUNDABOUT), this shouldn't happen with current node structure usually?
        // Actually, roundabouts are usually single nodes in this graph.
        // The graph structure: Intersection -> RoundaboutNode -> Intersection
        // 
        // Wait, the user wants "tourne sur le rond point".
        // In `CreateTestNetwork`:
        // Node* n5 = network.AddNode({350.0f, 0.1f, 0.0f}, ROUNDABOUT, 60.0f);
        // Roads connect TO n5.
        // So a path goes A -> n5 -> B.
        // We need to handle the transition A->n5 and n5->B specifically.
        
        // Standard segment sampling
        std::vector<Vector3> segmentPoints;
        for (int s = 0; s <= samplesPerSegment; ++s) {
            float t = s / (float)samplesPerSegment;
            Vector3 p = isReverse ? connecting->GetLanePosition(laneIdx, 1.0f - t) 
                                  : connecting->GetLanePosition(laneIdx, t);
            segmentPoints.push_back(p);
        }

        // Add to main list
        points.insert(points.end(), segmentPoints.begin(), segmentPoints.end());
        
        // --- ROUNDABOUT TRANSITION ---
        // If current 'endNode' (which is next 'startNode') is a ROUNDABOUT, we need to add an arc 
        // from the end of this segment to the start of the next segment.
        if (i < nodePath.size() - 2) {
            Node* nextNode = nodePath[i+1]; // = endNode
            Node* futureNode = nodePath[i+2];
            
            if (nextNode->GetType() == ROUNDABOUT) {
                // We are at 'endNode' (Roundabout). 
                // Previous segment ended at 'endNode'. Next segment starts at 'endNode'.
                // We need to bridge the gap between "incoming road at roundabout" and "outgoing road at roundabout".
                
                // Get tangent/direction of incoming road at the roundabout
                // The segmentPoints.back() is roughly at the roundabout edge (due to geometry trimming in RoadSegment).
                // Let's rely on geometry. 
                // Actually, RoadSegment geometry stops AT the radius.
                // We need to calculate the angle of entry and angle of exit.
                
                Vector3 center = nextNode->GetPosition();
                float radius = nextNode->GetRadius(); // Outer radius
                // We drive on the right, so counter-clockwise rotation usually (in right-hand traffic).
                // Or clockwise? Smart City usually implies right-hand traffic.
                // Roundabout flow is counter-clockwise.
                
                Vector3 pEntry = segmentPoints.back();
                
                // Find start of next segment
                RoadSegment* nextSeg = nullptr;
                for (const auto& segPtr : network.GetRoadSegments()) {
                    RoadSegment* s = segPtr.get();
                    if (!s) continue;
                    if ((s->GetStartNode() == nextNode && s->GetEndNode() == futureNode) ||
                        (s->GetStartNode() == futureNode && s->GetEndNode() == nextNode)) {
                        nextSeg = s;
                        break;
                    }
                }
                
                if (nextSeg) {
                    bool nextIsReverse = (nextSeg->GetEndNode() == nextNode);
                    int nextLane = nextSeg->GetLanes() > 0 ? nextSeg->GetLanes()/2 : 0;
                    if (nextIsReverse) nextLane = nextSeg->GetLanes() - 1;
                    
                    Vector3 pExit = nextIsReverse ? nextSeg->GetLanePosition(nextLane, 1.0f) // start of reverse is t=1? No, t=1 is End. Reverse means Start is End.
                                                  : nextSeg->GetLanePosition(nextLane, 0.0f);
                                                  
                    // Wait, GetLanePosition(t) reference:
                    // If isReverse (Start=EndNode, End=StartNode), we drive Front->Back? 
                    // Let's stick to:
                    // Current Path: A -> R. Segment A-R. End of A-R is at R.
                    // Next Path: R -> B. Segment R-B. Start of R-B is at R.
                    
                    // Actually, let's just take the point.
                    // Entry point is Last point of current segment.
                    // Exit point is First point of NEXT segment (which we haven't generated yet, but we can query).
                    
                    Vector3 exitP = nextIsReverse ? nextSeg->GetLanePosition(nextLane, 1.0f) 
                                                  : nextSeg->GetLanePosition(nextLane, 0.0f);

                    // Compute angles
                    float angleEntry = atan2f(pEntry.z - center.z, pEntry.x - center.x);
                    float angleExit = atan2f(exitP.z - center.z, exitP.x - center.x);
                    
                    // Normalize angles
                   while (angleExit <= angleEntry) angleExit += 2*PI;
                   // Wait, for Counter-Clockwise (CCW), Exit should be > Entry.
                   // If Exit < Entry, we add 2PI.
                   
                   // However, coordinate system matters. 
                   // Raylib: Z is forward. X is right.
                   // atan2(z, x).
                   
                   // Let's just interpolate along the circle.
                   // Ensure we go the "short" way? No, roundabouts are one-way. 
                   // MUST go CCW. 
                   // So we force Delta > 0.
                   
                   float diff = angleExit - angleEntry;
                   // If we are "just right" of the exit, we have to do a full circle? No.
                   // If Exit is -0.1 and Entry is 0.1. Exit < Entry. 
                   // We want to go 0.1 -> ... -> PI -> -PI -> -0.1.
                   // This is CCW? 
                   // 0 to PI is CCW.
                   
                   // Let's use logic:
                   // Target Angle = angleExit.
                   // Current Angle = angleEntry.
                   // We increment angle.
                   if (diff < 0) diff += 2*PI;
                   
                   // If diff is too small (e.g. going straight?), treat as straight?
                   // No, always arc.
                   
                   int arcSamples = (int)(diff * 10); // heuristic
                   if (arcSamples < 5) arcSamples = 5;
                   
                   for (int k = 1; k < arcSamples; ++k) {
                       float t = k / (float)arcSamples;
                       float a = angleEntry + diff * t;
                       // Radius might need to be adjusted to lane centerline?
                       // Interpolate radius from entry to exit distance
                       float distEntry = Vector3Distance(pEntry, center);
                       float distExit  = Vector3Distance(exitP, center);
                       float r = distEntry * (1.0f - t) + distExit * t;
                       
                       points.push_back({
                           center.x + r * cosf(a),
                           center.y, 
                           center.z + r * sinf(a)
                       });
                   }
                }
            }
        }
    }
    return points;
}

// ==================== CAMERA INITIALIZATION ====================
void InitCamera() {
    g_camera.position = {200.0f, 150.0f, 200.0f};
    g_camera.target = {0.0f, 0.0f, 0.0f};
    g_camera.up = {0.0f, 1.0f, 0.0f};
    g_camera.fovy = 60.0f;
    g_camera.projection = CAMERA_PERSPECTIVE;
    
    g_camState.mode = CAM_ORBITAL; 
    g_camState.angles = {45.0f * DEG2RAD, 35.0f * DEG2RAD};
    g_camState.distance = 300.0f;
    g_camState.focusPoint = {0.0f, 0.0f, 0.0f};
    g_camState.yaw = -90.0f;
    g_camState.pitch = 0.0f;
    g_camState.followedVehicleId = -1;
    g_camState.smoothTransition = true;
    g_camState.cinematicMode = false;
}

// ==================== CAMERA UPDATE ====================
void UpdateCamera(TrafficManager& trafficMgr, float dt) {
    // Changement de mode
    if (IsKeyPressed(KEY_ONE)) {
        g_camState.mode = CAM_ORBITAL;
        EnableCursor();
    }
    if (IsKeyPressed(KEY_TWO)) {
        g_camState.mode = CAM_FREE_FLY;
        DisableCursor();
    }
    if (IsKeyPressed(KEY_THREE)) {
        g_camState.mode = CAM_FOLLOW_VEHICLE;
        EnableCursor();
        if (!trafficMgr.getVehicles().empty()) {
            auto* firstCar = dynamic_cast<Car*>(trafficMgr.getVehicles()[0].get());
            if (firstCar) {
            g_camState.followedVehicleId = firstCar->getCarId();
        }
           
        }
    }
    
    // Toggle Mode Cinématique
    if (IsKeyPressed(KEY_C)) {
        g_camState.cinematicMode = !g_camState.cinematicMode;
    }
    
    // Reset caméra
    if (IsKeyPressed(KEY_R)) {
        InitCamera();
    }
    
    switch (g_camState.mode) {
        case CAM_ORBITAL:
            UpdateCameraOrbital(dt);
            break;
        case CAM_FREE_FLY:
            UpdateCameraFreeFly(dt);
            break;
        case CAM_FOLLOW_VEHICLE:
            UpdateCameraFollow(trafficMgr, dt);
            break;
    }
}

// ==================== ORBITAL CAMERA (RTS Style) ====================
void UpdateCameraOrbital(float dt) {
    // Zoom avec molette
    float wheel = GetMouseWheelMove();
    g_camState.distance = Clamp(g_camState.distance - wheel * 20.0f, 50.0f, 800.0f);
    
    // Rotation avec clic droit
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        Vector2 delta = GetMouseDelta();
        g_camState.angles.x -= delta.x * 0.005f;
        g_camState.angles.y = Clamp(g_camState.angles.y - delta.y * 0.005f, 
                                     0.1f, PI/2 - 0.1f);
    }
    
    // Rotation Verticale (Pitch) avec Clavier (O/P)
    if (IsKeyDown(KEY_O)) {
        g_camState.angles.y = Clamp(g_camState.angles.y + 1.5f * dt, 0.1f, PI/2 - 0.1f);
    }
    if (IsKeyDown(KEY_P)) {
        g_camState.angles.y = Clamp(g_camState.angles.y - 1.5f * dt, 0.1f, PI/2 - 0.1f);
    }
    
    // Animation Cinématique (Rotation automatique lente)
    if (g_camState.cinematicMode) {
        g_camState.angles.x += 0.5f * dt;
    }
    
    // Déplacement WASD/ZQSD (Pan)
    Vector3 forward = Vector3Normalize(Vector3{
        sinf(g_camState.angles.x), 0, cosf(g_camState.angles.x)
    });
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, {0, 1, 0}));
    
    float panSpeed = 100.0f;
    if (IsKeyDown(KEY_LEFT_SHIFT)) panSpeed = 200.0f;
    
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_Z)) {
        g_camState.focusPoint = Vector3Add(g_camState.focusPoint, 
                                           Vector3Scale(forward, panSpeed * dt));
    }
    if (IsKeyDown(KEY_S)) {
        g_camState.focusPoint = Vector3Subtract(g_camState.focusPoint, 
                                                Vector3Scale(forward, panSpeed * dt));
    }
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_Q)) {
        g_camState.focusPoint = Vector3Subtract(g_camState.focusPoint, 
                                                Vector3Scale(right, panSpeed * dt));
    }
    if (IsKeyDown(KEY_D)) {
        g_camState.focusPoint = Vector3Add(g_camState.focusPoint, 
                                           Vector3Scale(right, panSpeed * dt));
    }
    
    // Zoom avec E/Q
    if (IsKeyDown(KEY_E)) {
        g_camState.distance = Clamp(g_camState.distance + 150.0f * dt, 50.0f, 800.0f);
    }
    if (IsKeyDown(KEY_UP)) {
        g_camState.distance = Clamp(g_camState.distance - 150.0f * dt, 50.0f, 800.0f);
    }
    if (IsKeyDown(KEY_DOWN)) {
        g_camState.distance = Clamp(g_camState.distance + 150.0f * dt, 50.0f, 800.0f);
    }
    
    // Rotation avec flèches
    if (IsKeyDown(KEY_LEFT)) {
        g_camState.angles.x -= 1.5f * dt;
    }
    if (IsKeyDown(KEY_RIGHT)) {
        g_camState.angles.x += 1.5f * dt;
    }
    
    // Calculer position caméra
    g_camera.target = g_camState.focusPoint;
    g_camera.position = {
        g_camState.focusPoint.x + g_camState.distance * sinf(g_camState.angles.y) * cosf(g_camState.angles.x),
        g_camState.focusPoint.y + g_camState.distance * cosf(g_camState.angles.y),
        g_camState.focusPoint.z + g_camState.distance * sinf(g_camState.angles.y) * sinf(g_camState.angles.x)
    };
}

// ==================== FREE FLY CAMERA (FPS Style) ====================
void UpdateCameraFreeFly(float dt) {
    // Rotation souris
    Vector2 mouseDelta = GetMouseDelta();
    g_camState.yaw -= mouseDelta.x * 0.15f;
    g_camState.pitch -= mouseDelta.y * 0.15f;
    g_camState.pitch = Clamp(g_camState.pitch, -89.0f, 89.0f);
    
    // Direction
    Vector3 forward = {
        cosf(g_camState.pitch * DEG2RAD) * sinf(g_camState.yaw * DEG2RAD),
        sinf(g_camState.pitch * DEG2RAD),
        cosf(g_camState.pitch * DEG2RAD) * cosf(g_camState.yaw * DEG2RAD)
    };
    forward = Vector3Normalize(forward);
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, {0, 1, 0}));
    
    // Vitesse
    float speed = 50.0f;
    if (IsKeyDown(KEY_LEFT_SHIFT)) speed = 150.0f;
    if (IsKeyDown(KEY_LEFT_CONTROL)) speed = 20.0f;
    
    // Déplacement WASD/ZQSD
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_Z)) {
        g_camera.position = Vector3Add(g_camera.position, Vector3Scale(forward, speed * dt));
    }
    if (IsKeyDown(KEY_S)) {
        g_camera.position = Vector3Subtract(g_camera.position, Vector3Scale(forward, speed * dt));
    }
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_Q)) {
        g_camera.position = Vector3Subtract(g_camera.position, Vector3Scale(right, speed * dt));
    }
    if (IsKeyDown(KEY_D)) {
        g_camera.position = Vector3Add(g_camera.position, Vector3Scale(right, speed * dt));
    }
    
    // Monter/Descendre
    if (IsKeyDown(KEY_SPACE)) {
        g_camera.position.y += speed * dt;
    }
    if (IsKeyDown(KEY_LEFT_ALT)) {
        g_camera.position.y -= speed * dt;
        if (g_camera.position.y < 2.0f) g_camera.position.y = 2.0f;
    }
    
    g_camera.target = Vector3Add(g_camera.position, forward);
}
// ==================== FOLLOW VEHICLE CAMERA ====================
void UpdateCameraFollow(TrafficManager& trafficMgr, float dt) {
    const auto& vehicles = trafficMgr.getVehicles();
    
    // Changer de véhicule avec Tab
    if (IsKeyPressed(KEY_TAB) && !vehicles.empty()) {
        for (size_t i = 0; i < vehicles.size(); i++) {
            // ✅ Utiliser dynamic_cast pour convertir Vehicule* en Car*
            auto* carPtr = dynamic_cast<Car*>(vehicles[i].get());
            if (carPtr && carPtr->getCarId() == g_camState.followedVehicleId) {
                auto* nextCarPtr = dynamic_cast<Car*>(vehicles[(i + 1) % vehicles.size()].get());
                if (nextCarPtr) {
                    g_camState.followedVehicleId = nextCarPtr->getCarId();
                }
                break;
            }
        }
    }

    
    // Trouver le véhicule suivi
    Car* followedCar = nullptr;
    for (const auto& vehicle : vehicles) {  // ✅ Renommer 'car' en 'vehicle'
        auto* carPtr = dynamic_cast<Car*>(vehicle.get());  // ✅ Utiliser 'vehicle'
        if (carPtr && carPtr->getCarId() == g_camState.followedVehicleId) {
            followedCar = carPtr;
            break;
        }
    }

        // Si aucun véhicule trouvé, prendre le premier
    if (!followedCar && !vehicles.empty()) {
        followedCar = dynamic_cast<Car*>(vehicles[0].get());
        if (followedCar) {
            g_camState.followedVehicleId = followedCar->getCarId();
        }
    }
    
    if (followedCar) {
        Vector3 carPos = followedCar->getPosition();
        
        // Distance et hauteur ajustables
        float followDistance = 25.0f;
        float followHeight = 10.0f;
        
        if (IsKeyDown(KEY_E)) followDistance += 20.0f * dt;
        if (IsKeyDown(KEY_UP)) followDistance -= 20.0f * dt;
        followDistance = Clamp(followDistance, 10.0f, 100.0f);
        
        // Position caméra derrière le véhicule
        float carAngle = followedCar->getRotationAngle() * DEG2RAD;
        Vector3 offset = {
            -sinf(carAngle) * followDistance,
            followHeight,
            -cosf(carAngle) * followDistance
        };
        
        Vector3 targetPos = Vector3Add(carPos, offset);
        // Smooth transition
        if (g_camState.smoothTransition) {
            g_camera.position = Vector3Lerp(g_camera.position, targetPos, 5.0f * dt);
        } else {
            g_camera.position = targetPos;
        }
        
        g_camera.target = Vector3Add(carPos, {0, 2, 0});
    }
}
// ==================== ENVIRONNEMENT RÉALISTE ====================
void DrawEnvironment() {
    // CIEL BLEU (dégradé)
    ClearBackground(Color{135, 206, 235, 255}); // Sky blue
    
    // SOL VERT FONCÉ (demande utilisateur)
    DrawPlane({0, 0, 0}, {3000, 3000}, DARKGREEN);
    
    // Grille de référence (subtile)
    for (int i = -1500; i <= 1500; i += 100) {
        DrawLine3D({(float)i, 0.01f, -1500}, {(float)i, 0.01f, 1500}, 
                   Fade(DARKGRAY, 0.3f));
        DrawLine3D({-1500, 0.01f, (float)i}, {1500, 0.01f, (float)i}, 
                   Fade(DARKGRAY, 0.3f));
    }
    
    // Soleil (simulation)
    Vector3 sunPos = {500, 800, -500};
    DrawSphere(sunPos, 50, YELLOW);
    
    // Nuages (supprimés pour clarté)
    /*
    for (int i = 0; i < 10; i++) {
        float x = -800 + i * 200.0f;
        float z = -600 + (i % 3) * 300.0f;
        DrawSphere({x, 200, z}, 30, Fade(WHITE, 0.7f));
        DrawSphere({x + 20, 200, z + 10}, 25, Fade(WHITE, 0.6f));
    }
    */
    
    // Axes de référence
    
    // Axes de référence
    DrawLine3D({0, 0, 0}, {100, 0, 0}, RED);
    DrawLine3D({0, 0, 0}, {0, 100, 0}, GREEN);
    DrawLine3D({0, 0, 0}, {0, 0, 100}, BLUE);
}

// ==================== INFO CAMÉRA ====================
void DrawCameraInfo() {
    const char* modeText = "";
    switch (g_camState.mode) {
        case CAM_ORBITAL: modeText = "ORBITAL (RTS)"; break;
        case CAM_FREE_FLY: modeText = "FREE FLY (FPS)"; break;
        case CAM_FOLLOW_VEHICLE: modeText = "FOLLOW VEHICLE"; break;
    }
    
    DrawText(TextFormat("Camera: %s", modeText), 20, 130, 16, YELLOW);
    DrawText(TextFormat("Pos: (%.0f, %.0f, %.0f)", 
             g_camera.position.x, g_camera.position.y, g_camera.position.z),
             20, 150, 14, LIGHTGRAY);
}

// ==================== STRING → ENUM ====================
CarModel StringToCarModel(const std::string& name) {
    if (name == "Dodge Challenger") return CarModel::DODGE_CHALLENGER;
    if (name == "Chevrolet Camaro") return CarModel::CHEVROLET_CAMARO;
    if (name == "Nissan GTR") return CarModel::NISSAN_GTR;
    if (name == "SUV") return CarModel::SUV_MODEL;
    if (name == "Taxi") return CarModel::TAXI;
    if (name == "Truck") return CarModel::TRUCK;
    if (name == "Bus") return CarModel::BUS;
    if (name == "Ferrari") return CarModel::FERRARI;
    if (name == "Tesla") return CarModel::TESLA;
    if (name == "Convertible") return CarModel::CONVERTIBLE;
    if (name == "CarBlanc") return CarModel::CAR_BLANC;
    if (name == "Red Car") return CarModel::GENERIC_RED;
    if (name == "Model 1") return CarModel::GENERIC_MODEL_1;
    return CarModel::CAR_BLANC; // Default safe fallback
}

// ==================== ROAD NETWORK SETUP ====================
void CreateTestNetwork(RoadNetwork& network) {
    Node* n1 = network.AddNode({1050.0f, 0.0f, -450.0f}, SIMPLE_INTERSECTION, 20.0f);
    Node* n2 = network.AddNode({700.0f, 0.0f, -450.0f}, SIMPLE_INTERSECTION, 20.0f);
    Node* n3 = network.AddNode({700.0f, 0.0f, -250.0f}, SIMPLE_INTERSECTION, 8.0f);
    Node* n4 = network.AddNode({350.0f, 0.0f, -450.0f}, SIMPLE_INTERSECTION, 20.0f);
    Node* n5 = network.AddNode({350.0f, 0.1f, 0.0f}, ROUNDABOUT, 60.0f);
    Node* n6 = network.AddNode({0.0f, 0.1f, -450.0f}, ROUNDABOUT, 60.0f);
    Node* n7 = network.AddNode({0.0f, 0.0f, -600.0f}, SIMPLE_INTERSECTION, 20.0f);
    Node* n8 = network.AddNode({0.0f, 0.0f, 0.0f}, TRAFFIC_LIGHT, 25.0f);
    Node* n9 = network.AddNode({0.0f, 0.0f, 150.0f}, SIMPLE_INTERSECTION, 20.0f);
    Node* n10 = network.AddNode({-150.0f, 0.0f, 0.0f}, SIMPLE_INTERSECTION, 20.0f);
    
    network.AddRoadSegment(n1, n2, 4, false);
    network.AddRoadSegment(n2, n1, 4, false)->SetVisible(false); // Reverse
    
    network.AddRoadSegment(n2, n3, 2, false);
    network.AddRoadSegment(n3, n2, 2, false)->SetVisible(false); // Reverse
    
    network.AddRoadSegment(n2, n4, 4, false);
    network.AddRoadSegment(n4, n2, 4, false)->SetVisible(false); // Reverse
    
    network.AddRoadSegment(n4, n5, 4, false); // Roundabout entry
    // network.AddRoadSegment(n5, n4, 4, false)->SetVisible(false); // Exit? Roundabouts are usually one-way entry/exit logic.
    // Let's assume n4->n5 is entry. n5->n4 is exit?
    // User wants "tourne sur le rond point".
    // Usually Roundabout (n5) connects to other exits.
    
    network.AddRoadSegment(n4, n6, 4, false);
    network.AddRoadSegment(n6, n4, 4, false)->SetVisible(false);
    
    network.AddRoadSegment(n7, n6, 4, false);
    network.AddRoadSegment(n6, n7, 4, false)->SetVisible(false); // Reverse
    
    network.AddRoadSegment(n6, n8, 4, false);
    network.AddRoadSegment(n8, n6, 4, false)->SetVisible(false);
    
    network.AddRoadSegment(n8, n9, 4, false);
    network.AddRoadSegment(n9, n8, 4, false)->SetVisible(false);
    
    network.AddRoadSegment(n8, n10, 4, false);
    network.AddRoadSegment(n10, n8, 4, false)->SetVisible(false);
    
    network.AddRoadSegment(n5, n8, 4, false);
    
    network.AddIntersection(n1);
    network.AddIntersection(n2);
    network.AddIntersection(n3);
    network.AddIntersection(n4);
    network.AddIntersection(n5);
    network.AddIntersection(n6);
    network.AddIntersection(n7);
    network.AddIntersection(n8);
    network.AddIntersection(n9);
    network.AddIntersection(n10);
}

// ==================== TREE GENERATION ====================
std::vector<Vector3> GenerateTreesOnSidewalks(const RoadNetwork& network, float spacing) {
    std::vector<Vector3> positions;
    for (const auto& segPtr : network.GetRoadSegments()) {
        const auto& sidewalks = segPtr->GetSidewalks();
        for (const auto& sw : sidewalks) {
            if (sw.path.size() < 2) continue;
            
            float distAccum = 0.0f;
            float lastTreeDist = -spacing; // Start a bit earlier or right at the start
            
            for (size_t i = 1; i < sw.path.size(); ++i) {
                Vector3 start = sw.path[i-1];
                Vector3 end = sw.path[i];
                float segmentLen = Vector3Distance(start, end);
                
                while (lastTreeDist + spacing <= distAccum + segmentLen) {
                    float t = (lastTreeDist + spacing - distAccum) / segmentLen;
                    if (t < 0) t = 0;
                    if (t > 1) t = 1;
                    
                    Vector3 treePos = Vector3Lerp(start, end, t);
                    positions.push_back(treePos);
                    lastTreeDist += spacing;
                }
                
                distAccum += segmentLen;
            }
        }
    }
    return positions;
}

// ==================== MAIN ====================
int main() {
    InitWindow(1600, 900, "SMART CITY - Traffic Core Simulator");
    SetTargetFPS(60);
    
    InitCamera();
    
    TrafficManager trafficMgr;
    RoadNetwork network;
    CreateTestNetwork(network);
    
    // Centrer caméra sur réseau
    Vector3 networkCenter = {0, 0, 0};
    int nodeCount = 0;
    for (const auto& nPtr : network.GetNodes()) {
        networkCenter = Vector3Add(networkCenter, nPtr->GetPosition());
        nodeCount++;
    }
    if (nodeCount > 0) {
        networkCenter = Vector3Scale(networkCenter, 1.0f / (float)nodeCount);
        g_camState.focusPoint = networkCenter;
        g_camera.target = networkCenter;
    }
    
    // Charger modèles 3D
    ModelManager& mm = ModelManager::getInstance();
    std::vector<std::string> candidateDirs = {
        "assets/models",
        "../assets/models",
        "../../assets/models",
        "../TrafficCore/assets/models",
        "TrafficCore/assets/models"
    };
    
    for (const auto& dir : candidateDirs) {
        try {
            if (!std::filesystem::exists(dir)) continue;
            for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                if (!entry.is_regular_file()) continue;
                auto path = entry.path();
                if (path.has_extension() && 
                    (path.extension() == ".glb" || path.extension() == ".GLB")) {
                    std::string filename = path.filename().string();
                    std::string lowercase = filename;
                    std::transform(lowercase.begin(), lowercase.end(), 
                                 lowercase.begin(), ::tolower);
                    
                    // Exclude fountain and school from vehicle system
                    if (lowercase.find("fountain") != std::string::npos) continue;
                    if (lowercase.find("school") != std::string::npos) continue;
                    if (lowercase.find("sh") != std::string::npos) continue; // Exclude SH module
                    if (lowercase.find("tree") != std::string::npos) continue; // Exclude Tree assets
                    if (lowercase.find("plant") != std::string::npos) continue; // Exclude Plant assets
                    if (lowercase.find("abribus") != std::string::npos) continue; // Exclude Abribus assets
                    if (lowercase.find("bus_station") != std::string::npos) continue;
                    if (lowercase.find("busstop") != std::string::npos) continue; // Exclude Bus Stop assets
                    if (lowercase.find("stop") != std::string::npos) continue; // Exclude Bus Stop assets
                    if (lowercase.find("building") != std::string::npos) continue; // Exclude Building assets
                    if (lowercase.find("stadium") != std::string::npos) continue; // Exclude Stadium asset
                    if (lowercase.find("restaurant") != std::string::npos) continue; // Exclude Restaurant
                    if (lowercase.find("hotel") != std::string::npos) continue; // Exclude Hotel

                    // Filter: Only exclude explicitly bad files if known, otherwise load user's folder
                    // User cleaned folder, so we load everything present.
                    // if (lowercase.find("bad_file") != std::string::npos) continue;

                    std::string category = "CAR";
                    if (lowercase.find("truck") != std::string::npos) category = "TRUCK";
                    else if (lowercase.find("bus") != std::string::npos) category = "BUS";
                    
                    mm.loadModel(category, std::filesystem::absolute(path).string());
                }
            }
            break;
        } catch (...) {}
    }
    
    // Créer véhicules initiaux
    std::vector<std::string> availableModels = {
        "Tesla", "Truck", "Convertible", "CarBlanc", "Red Car", "Model 1"
    };
    
    int nextCarId = 1;
    int sampleCountPerSegment = 12;
    const auto& nodes = network.GetNodes();
    int desiredVehicles = (int)network.GetRoadSegmentCount() * 6; // High density
    int vehiclesCreated = 0;
    int maxAttempts = desiredVehicles * 5;
    int attempts = 0;
    
        while (vehiclesCreated < desiredVehicles && attempts < maxAttempts) {
            attempts++;
            if (nodes.empty()) break;
            
            Node* start = nodes[GetRandomValue(0, (int)nodes.size()-1)].get();
            Node* end = start;
            int tries = 0;
            while (end == start && tries < 10) {
                end = nodes[GetRandomValue(0, (int)nodes.size()-1)].get();
                tries++;
            }
            
            auto nodePath = network.FindPath(start, end);
            if (nodePath.size() < 2) continue;
            
            // Use Helper
            std::vector<Vector3> pathPoints = GeneratePathPoints(network, nodePath, sampleCountPerSegment);
            
            if (pathPoints.size() < 2) continue;
            
            // Safety Check
            bool safe = true;
            for (const auto& otherPos : trafficMgr.getVehiclePositions()) {
                 if (Vector3Distance(pathPoints.front(), otherPos) < 10.0f) {
                     safe = false;
                     break;
                 }
            }
            if (!safe) continue;

            auto car = CarFactory::createCar(
                StringToCarModel(availableModels[GetRandomValue(0, availableModels.size()-1)]), 
                pathPoints.front(), 
                nextCarId++
            );
            car->normalizeSize(10.0f); // FORCE UNIFORM LARGE SIZE (10 units)
            car->setItinerary(pathPoints);
            trafficMgr.addVehicle(std::move(car));
            vehiclesCreated++;
        }

    // --- LOAD FOUNTAIN MODEL ---
    Model fountainModel = LoadModel("../TrafficCore/assets/models/fountain_water_simulation.glb");
    Vector3 fountainPos = {0.0f, 0.1f, -450.0f}; // Node n6 position
    float fountainScale = 45.0f; // Légèrement augmenté

    // --- LOAD SCHOOL MODEL ---
    Model schoolModel = LoadModel("../TrafficCore/assets/models/school_building.glb");
    Vector3 schoolPos = {900.0f, 0.0f, -250.0f}; // Ajusté: "plus haut en z à l'extrémité"
    float schoolScale = 150.0f; // Augmenté značablement (était 80.0f)

    // --- LOAD SH MODEL (Using Tree.glb on user request) ---
    Model shModel = LoadModel("../TrafficCore/assets/models/Tree.glb");
    Vector3 shPos = {525.0f, 0.0f, -125.0f}; // Between n3 and n5
    float shScale = 20.0f; // Adjusted scale for a single tree module

    // --- LOAD TREES ---
    Model treeModel = LoadModel("../TrafficCore/assets/models/Tree.glb");
    float treeSpacing = 60.0f; // Distance entre les arbres
    std::vector<Vector3> treePositions = GenerateTreesOnSidewalks(network, treeSpacing);
    
    // --- LOAD PLANTS ---
    Model plantModel = LoadModel("../TrafficCore/assets/models/Plant Big.glb");
    Vector3 n5Pos = {350.0f, 0.1f, 0.0f}; // Position du rond-point n5
    std::vector<Vector3> plantPositions;
    for (int i = 0; i < 8; i++) {
        float angle = i * (360.0f / 8.0f) * DEG2RAD;
        float radius = 25.0f;
        plantPositions.push_back({
            n5Pos.x + cosf(angle) * radius,
            n5Pos.y,
            n5Pos.z + sinf(angle) * radius
        });
    }
    // Ajouter un au centre
    plantPositions.push_back(n5Pos);

    // --- LOAD ABRIBUS ---
    Model abribusModel = LoadModel("../TrafficCore/assets/models/abribus_bus_stop_bus_station.glb");
    Vector3 abribusPos = {313.6f, 0.0f, -80.0f}; // De l'autre côté de la route (X=350 - 36.4)
    float abribusRotation = 0.0f; // Face à la route

    // --- LOAD BUS STOP (Beside Abribus) ---
    Model busStopModel2 = LoadModel("../TrafficCore/assets/models/busstop.glb");
    Vector3 busStop2Pos = { abribusPos.x, abribusPos.y, abribusPos.z - 15.0f }; // À gauche (décalé en Z)
    float busStop2Rotation = 0.0f;

    // --- LOAD LARGE BUILDINGS ---
    Model buildingModel0 = LoadModel("../TrafficCore/assets/models/Large Building 0.glb");
    Vector3 building0Pos = { 110.0f, 0.0f, 75.0f }; // Entre n8 (0,0,0) et n9 (0,0,150)
    float building0Scale = 60.0f;

    Model stadiumModel = LoadModel("../TrafficCore/assets/models/stadium.glb");
    Vector3 stadiumPos = { 265.0f, 0.0f, -55.0f }; // X: 250, Z: -75
    float stadiumScale = 8.0f; 

    // --- LOAD RESTAURANT ---
    Model restaurantModel = LoadModel("../TrafficCore/assets/models/restaurant.glb");
    Vector3 restaurantPos = { 60.0f, 0.0f, -225.0f }; // Between n8 (0,0,0) and n6 (0,0,-450)
    float restaurantScale = 5.0f;

    // --- LOAD HOTEL ---
    Model hotelModel = LoadModel("../TrafficCore/assets/models/hotel.glb");
    Vector3 hotelPos = { -75.0f, 0.0f, 35.0f }; // Near n8 and n10, offset from road
    float hotelScale = 15.0f;

    bool paused = false;
    float simTime = 0.0f;

    Model buildingModel3 = LoadModel("../TrafficCore/assets/models/Large Building 3.glb");
    float building3Scale = 40.0f;
    std::vector<Vector3> building3Positions;
    // Row between n2 (x=700) and n4 (x=350)
    for (float x = 360.0f; x <= 700.0f; x += 45.0f) {
        building3Positions.push_back({ x, 0.0f, -520.0f });
    }

    // ==================== LOOP ====================
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (!paused) simTime += dt;
        
        // Input
        if (IsKeyPressed(KEY_SPACE)) paused = !paused;

        // Caméra
        UpdateCamera(trafficMgr, dt);

        // Ajouter véhicule
        if (IsKeyPressed(KEY_V) && !nodes.empty()) {
            Node* randomNode = nodes[GetRandomValue(0, nodes.size()-1)].get();
            Vector3 pos = randomNode->GetPosition();
            auto car = CarFactory::createCar(
                StringToCarModel(availableModels[GetRandomValue(0, availableModels.size()-1)]),
                pos,
                nextCarId++
            );
            car->normalizeSize(10.0f); // FORCE UNIFORM LARGE SIZE
            trafficMgr.addVehicle(std::move(car));
        }

        // Supprimer véhicule (Touche K)
        if (IsKeyPressed(KEY_K) && trafficMgr.getVehicleCount() > 0) {
            auto& vehicles = trafficMgr.getVehiclesCheck();
            bool removed = false;
            
            // Si on suit un véhicule, on supprime celui-là
            if (g_camState.mode == CAM_FOLLOW_VEHICLE && g_camState.followedVehicleId != -1) {
                for (auto it = vehicles.begin(); it != vehicles.end(); ++it) {
                    Car* carPtr = dynamic_cast<Car*>(it->get());
                    if (carPtr && carPtr->getCarId() == g_camState.followedVehicleId) {
                        vehicles.erase(it);
                        g_camState.followedVehicleId = -1; // Reset selection
                        removed = true;
                        break;
                    }
                }
            }
            
            // Sinon on supprime le dernier ajouté
            if (!removed && !vehicles.empty()) {
                vehicles.pop_back();
            }
        }
        
        // Update
        if (!paused) {
            trafficMgr.update(dt);
            
            // 1. Remove finished vehicles & Respawn
            trafficMgr.removeFinishedVehicles();
            const auto& vehicles = trafficMgr.getVehicles();
            
            // 2. Auto-spawn to maintain density
            if (vehicles.size() < desiredVehicles && !nodes.empty()) {
                // Try to spawn a new one
                 Node* start = nodes[GetRandomValue(0, (int)nodes.size()-1)].get();
                 Node* end = start;
                 int tries = 0;
                 while (end == start && tries < 10) {
                     end = nodes[GetRandomValue(0, (int)nodes.size()-1)].get();
                     tries++;
                 }
                 
                 // Reuse path generation logic? Ideally refactor into function.
                 // For now, duplicate or quick inline check
                 auto nodePath = network.FindPath(start, end);
                 if (nodePath.size() >= 2) {
                     std::vector<Vector3> pathPoints = GeneratePathPoints(network, nodePath, sampleCountPerSegment);
                     
                     if (pathPoints.size() >= 2) {
                         // Check safety
                         bool safe = true;
                         Vector3 spawnPos = pathPoints.front();
                         for (const auto& v : vehicles) {
                             if (Vector3Distance(v->getPosition(), spawnPos) < 15.0f) {
                                 safe = false;
                                 break;
                             }
                         }
                         
                         if (safe) {
                             auto car = CarFactory::createCar(
                                 StringToCarModel(availableModels[GetRandomValue(0, availableModels.size()-1)]),
                                 spawnPos,
                                 nextCarId++
                             );
                             
                             // Adjust size based on model type
                             float size = 10.0f; // Standard car size
                             if (car->getModelType() == CarModel::GENERIC_MODEL_1 || 
                                 car->getModelType() == CarModel::TRUCK) {
                                 size = 16.0f; // Bigger for Bus/Truck
                             }
                             
                             car->normalizeSize(size);
                             car->setItinerary(pathPoints);
                             trafficMgr.addVehicle(std::move(car));
                         }
                     }
                 }
            }
        }
        
        // ==================== DRAW ====================
        BeginDrawing();
        
        BeginMode3D(g_camera);
            DrawEnvironment();
            network.Draw();
            
            // Draw Blue Circle Base (Water/Pool)
            DrawCylinder(fountainPos, 40.0f, 40.0f, 1.0f, 32, BLUE);

            // Draw Fountain
            DrawModel(fountainModel, fountainPos, fountainScale, WHITE);
            
            // Draw School
            DrawModel(schoolModel, schoolPos, schoolScale, WHITE);
            
            // Draw SH Module
            DrawModel(shModel, shPos, shScale, WHITE);
            
            // Draw Trees
            for (const auto& pos : treePositions) {
                DrawModel(treeModel, pos, 6.0f, WHITE); // Taille réduite (6.0f)
            }

            // Draw Large Building 0
            DrawModel(buildingModel0, building0Pos, building0Scale, WHITE);

            // Draw Stadium (Rotated 90 degrees)
            DrawModelEx(stadiumModel, stadiumPos, {0, 1, 0}, 90.0f, {stadiumScale, stadiumScale, stadiumScale}, WHITE);

            // Draw Restaurant
            DrawModelEx(restaurantModel, restaurantPos, { 0.0f, 1.0f, 0.0f }, 180.0f, { restaurantScale, restaurantScale, restaurantScale }, WHITE);

            // Draw Hotel
            DrawModelEx(hotelModel, hotelPos, { 0.0f, 1.0f, 0.0f }, 0.0f, { hotelScale, hotelScale, hotelScale }, WHITE);

            // Draw Large Building 3 instances
            for (const auto& pos : building3Positions) {
                DrawModel(buildingModel3, pos, building3Scale, WHITE);
            }

            // Draw Plants at n5
            for (const auto& pos : plantPositions) {
                DrawModel(plantModel, pos, 15.0f, WHITE);
            }

            // Draw Abribus
            DrawModelEx(abribusModel, abribusPos, {0, 1, 0}, abribusRotation, {5.0f, 5.0f, 5.0f}, WHITE);

            // Draw Bus Stop (next to Abribus)
            DrawModelEx(busStopModel2, busStop2Pos, {0, 1, 0}, busStop2Rotation, {8.0f, 8.0f, 8.0f}, WHITE);

            for (const auto& car : trafficMgr.getVehicles()) {
                car->draw();
            }
        EndMode3D();
        
        // UI
        DrawUIPanel(10, 10, 350, 180, "SMART CITY - BOUKHALF 2030");
        DrawText(TextFormat("Vehicles: %d", trafficMgr.getVehicleCount()), 
                 20, 50, 18, GREEN);
        DrawText(TextFormat("Time: %.1f s", simTime), 20, 75, 16, SKYBLUE);
        DrawText(paused ? "PAUSED" : "RUNNING", 20, 100, 16, paused ? RED : GREEN);
        DrawCameraInfo();
        
        DrawUIPanel(10, 200, 350, 320, "CONTROLS");
        DrawText("SPACE     : Pause/Resume", 20, 225, 13, WHITE);
        DrawText("V / K     : Add / Delete Vehicle", 20, 243, 13, WHITE);
        DrawText("R         : Reset Camera", 20, 261, 13, WHITE);
        DrawText("C         : Cinematic Mode (Auto-Orbit)", 20, 279, 13, SKYBLUE);
        
        DrawText("1 / 2 / 3 : Camera Modes", 20, 310, 13, YELLOW);
        DrawText("  1 = Orbital (RTS)", 20, 326, 12, LIGHTGRAY);
        DrawText("  2 = Free Fly (FPS)", 20, 340, 12, LIGHTGRAY);
        DrawText("  3 = Follow Vehicle", 20, 354, 12, LIGHTGRAY);
        
        DrawText("KEYBOARD CONTROLS", 20, 385, 13, YELLOW);
        DrawText("WASD/ZQSD : Move Focus (Orbital)", 20, 403, 12, GREEN);
        DrawText("ARROWS    : Rotate (H)", 20, 418, 12, GREEN);
        DrawText("O / P     : Rotate (V / Pitch)", 20, 433, 12, GREEN);
        DrawText("E / Q     : Zoom In / Out", 20, 448, 12, GREEN);
        DrawText("TAB       : Switch Follow Vehicle", 20, 463, 12, GREEN);
        DrawText("SHIFT     : Speed Boost", 20, 478, 12, GREEN);
        
        DrawFPS(1450, 870);
        DrawText("FIFA World Cup 2030", 1350, 20, 16, MAROON);
        
        EndDrawing();
    }
    
    EnableCursor();
    CloseWindow();
    return 0;
}

// ==================== UI ====================
void DrawUIPanel(int x, int y, int width, int height, const char* title) {
    DrawRectangle(x, y, width, height, Fade(BLACK, 0.8f));
    DrawRectangleLines(x, y, width, height, YELLOW);
    DrawText(title, x + 10, y + 8, 16, YELLOW);
}