// demos/boukhalf_simulation.cpp
#include <utility>
#include "raylib.h"
#include "raymath.h"
#include "Vehicules/VehiculeFactory.h"
#include "Vehicules/TrafficManager.h"
#include "Vehicules/Car.h"
#include "RoadNetwork.h"

#include <memory>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <vector>
#include <string>
#include <cmath>
#include <filesystem>
#include "Vehicules/ModelManager.h"
#include "PathFinder.h"
#include <map>
#include "MapLoader.h"
#include <random>
#include <ctime>

// Small Catmull-Rom spline helper for smoothing paths used by GeneratePathPoints
static Vector3 demoCatmullRomInterpolate(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;
    Vector3 out;
    out.x = 0.5f * ((2.0f * p1.x) + (-p0.x + p2.x) * t + (2.0f*p0.x - 5.0f*p1.x + 4.0f*p2.x - p3.x) * t2 + (-p0.x + 3.0f*p1.x - 3.0f*p2.x + p3.x) * t3);
    out.y = 0.5f * ((2.0f * p1.y) + (-p0.y + p2.y) * t + (2.0f*p0.y - 5.0f*p1.y + 4.0f*p2.y - p3.y) * t2 + (-p0.y + 3.0f*p1.y - 3.0f*p2.y + p3.y) * t3);
    out.z = 0.5f * ((2.0f * p1.z) + (-p0.z + p2.z) * t + (2.0f*p0.z - 5.0f*p1.z + 4.0f*p2.z - p3.z) * t2 + (-p0.z + 3.0f*p1.z - 3.0f*p2.z + p3.z) * t3);
    return out;
}

static std::vector<Vector3> demoCatmullRomChain(const std::vector<Vector3>& pts, int samplesPerSeg) {
    std::vector<Vector3> out;
    if (pts.size() < 2) return pts;
    std::vector<Vector3> p;
    p.push_back(pts.front());
    for (auto &v : pts) p.push_back(v);
    p.push_back(pts.back());
    for (size_t i = 0; i + 3 < p.size(); ++i) {
        const Vector3 &p0 = p[i];
        const Vector3 &p1 = p[i+1];
        const Vector3 &p2 = p[i+2];
        const Vector3 &p3 = p[i+3];
        for (int s = 0; s < samplesPerSeg; ++s) {
            float t = s / (float)samplesPerSeg;
            out.push_back(demoCatmullRomInterpolate(p0,p1,p2,p3,t));
        }
    }
    out.push_back(pts.back());
    return out;
}

// ==================== CONFIGURATION STRUCTURE ====================
struct SimulationConfig {
    std::map<std::string, int> vehicleCounts;
    bool isConfigured = false;

    // Selected model paths for each vehicle kind (absolute or relative)
    std::string selectedModelCar;
    std::string selectedModelBus;
    std::string selectedModelTruck;
};
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
// Forward declaration for config loader initializer
void InitializeNetworkFromConfig(RoadNetwork& network);
void DrawEnvironment();
void DrawCameraInfo();
void DrawUIPanel(int x, int y, int width, int height, const char* title);
SimulationConfig ShowConfigurationMenu();

CarModel StringToCarModel(const std::string& name);
void CreateTestNetwork(RoadNetwork& network);
std::vector<Vector3> GenerateTreesOnSidewalks(const RoadNetwork& network, float spacing);
std::vector<Vector3> GeneratePathPoints(const RoadNetwork& network, const std::vector<Node*>& nodePath, int samplesPerSegment);

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
                // Deterministic choice: use the first of the forward lanes (minLane)
                int minLane = lanes / 2;
                laneIdx = minLane;
            } else {
                // Deterministic choice for reverse: use the first of reverse lanes (minLane == 0)
                int minLane = 0;
                laneIdx = minLane;
            }
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
                    
                    Vector3 exitP = nextIsReverse ? nextSeg->GetLanePosition(nextLane, 1.0f) 
                                                  : nextSeg->GetLanePosition(nextLane, 0.0f);

                    // Compute angles
                    float angleEntry = atan2f(pEntry.z - center.z, pEntry.x - center.x);
                    float angleExit = atan2f(exitP.z - center.z, exitP.x - center.x);
                    
                    while (angleExit <= angleEntry) angleExit += 2*PI;
                   
                    float diff = angleExit - angleEntry;
                    if (diff < 0) diff += 2*PI;
                    
                    int arcSamples = (int)(diff * 30); // higher resolution for smoothness
                    if (arcSamples < 8) arcSamples = 8;

                    // Determine roundabout lane parameters
                    const float laneWidth = 16.0f; // match RoadSegment default
                    int incomingLanes = connecting->GetLanes() > 0 ? connecting->GetLanes() : 1;
                    int outgoingLanes = nextSeg->GetLanes() > 0 ? nextSeg->GetLanes() : 1;
                    int totalRoundaboutLanes = std::max(2, std::max(incomingLanes, outgoingLanes));

                    // Map incoming lane index to a roundabout lane index (clamp)
                    int roundaboutLaneIdx = laneIdx;
                    if (roundaboutLaneIdx < 0) roundaboutLaneIdx = 0;
                    if (roundaboutLaneIdx >= totalRoundaboutLanes) roundaboutLaneIdx = totalRoundaboutLanes - 1;

                    float roadWidth = totalRoundaboutLanes * laneWidth;
                    float laneOffset = -roadWidth / 2.0f + roadWidth * (0.5f + roundaboutLaneIdx) / (float)totalRoundaboutLanes;

                    // Compute radius for the chosen roundabout lane
                    float outerR = radius; // outer radius from node
                    float laneRadius = outerR - laneOffset;

                    for (int k = 1; k < arcSamples; ++k) {
                        float t = k / (float)arcSamples;
                        float a = angleEntry + diff * t;
                        points.push_back({
                            center.x + laneRadius * cosf(a),
                            center.y,
                            center.z + laneRadius * sinf(a)
                        });
                    }
                }
            }
        }
    }
    // Smooth path for fluid turns
    points = demoCatmullRomChain(points, samplesPerSegment);
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
    if (name == "Taxi") return CarModel::TAXI;
    if (name == "Convertible") return CarModel::CONVERTIBLE;
    if (name == "CarBlanc") return CarModel::CAR_BLANC;
    if (name == "Model 1") return CarModel::GENERIC_MODEL_1;
    return CarModel::CAR_BLANC; // Default safe fallback
}

// ==================== ROAD NETWORK SETUP ====================
void CreateTestNetwork(RoadNetwork& network) {
    Node* n1 = network.AddNode({1050.0f, 0.2f, -450.0f}, SIMPLE_INTERSECTION, 20.0f);
    Node* n2 = network.AddNode({700.0f, 0.2f, -450.0f}, SIMPLE_INTERSECTION, 20.0f);
    Node* n3 = network.AddNode({700.0f, 0.2f, -250.0f}, SIMPLE_INTERSECTION, 8.0f);
    Node* n4 = network.AddNode({350.0f, 0.2f, -450.0f}, SIMPLE_INTERSECTION, 20.0f);
    Node* n5 = network.AddNode({350.0f, 0.2f, 0.0f}, ROUNDABOUT, 60.0f);
    Node* n6 = network.AddNode({0.0f, 0.2f, -450.0f}, ROUNDABOUT, 60.0f);
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
    
    network.AddRoadSegment(n4, n5, 4, false); 
    network.AddRoadSegment(n5, n4, 4, false)->SetVisible(false);
    
    network.AddRoadSegment(n4, n6, 4, false);
    network.AddRoadSegment(n6, n4, 4, false)->SetVisible(false);
    
    network.AddRoadSegment(n7, n6, 4, false);
    network.AddRoadSegment(n6, n7, 4, false)->SetVisible(false); 
    
    network.AddRoadSegment(n6, n8, 4, false);
    network.AddRoadSegment(n8, n6, 4, false)->SetVisible(false);
    
    network.AddRoadSegment(n8, n9, 4, false);
    network.AddRoadSegment(n9, n8, 4, false)->SetVisible(false);
    
    network.AddRoadSegment(n8, n10, 4, false);
    network.AddRoadSegment(n10, n8, 4, false)->SetVisible(false);
    
    network.AddRoadSegment(n5, n8, 4, false);
    network.AddRoadSegment(n8, n5, 4, false)->SetVisible(false); // Link N8 -> N5 added
    
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


// ==================== CONFIGURATION MENU ====================
SimulationConfig ShowConfigurationMenu() {
    SimulationConfig config;
    
    // Only present three vehicle types in the UI
    std::vector<std::string> vehicleTypes = { "Car", "Bus", "Truck" };

    // Initialiser les compteurs à 0
    for (const auto& type : vehicleTypes) config.vehicleCounts[type] = 0;

    // Prepare model lists by scanning assets/models (filter into categories)
    std::vector<std::string> carModels;
    std::vector<std::string> busModels;
    std::vector<std::string> truckModels;
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
                if (path.has_extension() && (path.extension() == ".glb" || path.extension() == ".GLB")) {
                    std::string filename = path.filename().string();
                    std::string lower = filename;
                    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c){ return std::tolower(c); });

                    // Classify by keyword in filename while excluding common non-vehicle assets
                    if (lower.find("bus") != std::string::npos && lower.find("busstop") == std::string::npos && lower.find("bus_station") == std::string::npos && lower.find("abribus") == std::string::npos && lower.find("stop") == std::string::npos) {
                        // Accept many bus model filenames
                        busModels.push_back(std::filesystem::absolute(path).string());
                    } else if (lower.find("truck") != std::string::npos) {
                        truckModels.push_back(std::filesystem::absolute(path).string());
                    } else if (lower.find("taxi") != std::string::npos || lower.find("car") != std::string::npos || lower.find("convertible") != std::string::npos || lower.find("model") != std::string::npos) {
                        // Broad car classifier to include more car models (but may include other vehicles named 'car')
                        carModels.push_back(std::filesystem::absolute(path).string());
                    }
                }
            }
            break;
        } catch (...) {}
    }

    // Variables UI
    int screenWidth = 1200;
    int screenHeight = 800;
    InitWindow(screenWidth, screenHeight, "Configuration de la Simulation");
    SetTargetFPS(60);
    
    int startY = 150;
    int lineHeight = 110; // slightly larger to fit model selector
    int sliderWidth = 300;
    int sliderHeight = 20;
    
    bool startSimulation = false;

    // Persistent selected model indices (scoped to the menu)
    static int selectedCarIndex = 0;
    static int selectedBusIndex = 0;
    static int selectedTruckIndex = 0;
    
    while (!WindowShouldClose() && !startSimulation) {
        BeginDrawing();
        ClearBackground(Color{30, 30, 40, 255});
        
        // Titre
        DrawText("CONFIGURATION DE LA SIMULATION", 
                 screenWidth/2 - MeasureText("CONFIGURATION DE LA SIMULATION", 40)/2, 
                 40, 40, RAYWHITE);
        
DrawText("Choisissez le nombre de vehicules pour chaque type:", 
             screenWidth/2 - MeasureText("Choisissez le nombre de vehicules pour chaque type:", 20)/2, 
             100, 20, LIGHTGRAY);

    int totalVehicles = 0;

    for (size_t i = 0; i < vehicleTypes.size(); ++i) {
        const std::string& type = vehicleTypes[i];
        int y = startY + (int)i * lineHeight;
        int x = 100;

        DrawText(type.c_str(), x, y + 5, 24, RAYWHITE);

        Rectangle sliderBg = {(float)(x + 250), (float)y, (float)sliderWidth, (float)sliderHeight};
        Rectangle sliderFill = {
            (float)(x + 250),
            (float)y,
            (float)(sliderWidth * config.vehicleCounts[type] / 50.0f),
            (float)sliderHeight
        };
        DrawRectangleRec(sliderBg, DARKGRAY);
        DrawRectangleRec(sliderFill, SKYBLUE);
        DrawRectangleLinesEx(sliderBg, 2, LIGHTGRAY);

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 mousePos = GetMousePosition();
            if (CheckCollisionPointRec(mousePos, sliderBg)) {
                float relativeX = mousePos.x - sliderBg.x;
                config.vehicleCounts[type] = (int)((relativeX / sliderWidth) * 50);
                if (config.vehicleCounts[type] < 0) config.vehicleCounts[type] = 0;
                if (config.vehicleCounts[type] > 50) config.vehicleCounts[type] = 50;
            }
        }

        DrawText(TextFormat("%d", config.vehicleCounts[type]), x + 250 + sliderWidth + 20, y + 5, 24, YELLOW);

        int modelY = y + 30;
        std::string modelName = "(none)";
        if (type == "Car") {
            if (!carModels.empty()) { selectedCarIndex = std::clamp(selectedCarIndex, 0, (int)carModels.size()-1); modelName = std::filesystem::path(carModels[selectedCarIndex]).filename().string(); }
        } else if (type == "Bus") {
            if (!busModels.empty()) { selectedBusIndex = std::clamp(selectedBusIndex, 0, (int)busModels.size()-1); modelName = std::filesystem::path(busModels[selectedBusIndex]).filename().string(); }
        } else if (type == "Truck") {
            if (!truckModels.empty()) { selectedTruckIndex = std::clamp(selectedTruckIndex, 0, (int)truckModels.size()-1); modelName = std::filesystem::path(truckModels[selectedTruckIndex]).filename().string(); }
        }

        DrawText("Model:", x, modelY, 18, LIGHTGRAY);
        DrawText(modelName.c_str(), x + 80, modelY, 18, RAYWHITE);

        Rectangle prevBtn = {(float)(x + 200), (float)modelY, 30, 20};
        Rectangle nextBtn = {(float)(x + 240), (float)modelY, 30, 20};
        DrawRectangleRec(prevBtn, DARKGRAY); DrawRectangleRec(nextBtn, DARKGRAY);
        DrawText("<", prevBtn.x + 8, prevBtn.y + 2, 14, RAYWHITE); DrawText(">", nextBtn.x + 8, nextBtn.y + 2, 14, RAYWHITE);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 mp = GetMousePosition();
            if (CheckCollisionPointRec(mp, prevBtn)) { if (type == "Car" && !carModels.empty()) selectedCarIndex = (selectedCarIndex - 1 + (int)carModels.size()) % carModels.size(); if (type == "Bus" && !busModels.empty()) selectedBusIndex = (selectedBusIndex - 1 + (int)busModels.size()) % busModels.size(); if (type == "Truck" && !truckModels.empty()) selectedTruckIndex = (selectedTruckIndex - 1 + (int)truckModels.size()) % truckModels.size(); }
            if (CheckCollisionPointRec(mp, nextBtn)) { if (type == "Car" && !carModels.empty()) selectedCarIndex = (selectedCarIndex + 1) % carModels.size(); if (type == "Bus" && !busModels.empty()) selectedBusIndex = (selectedBusIndex + 1) % busModels.size(); if (type == "Truck" && !truckModels.empty()) selectedTruckIndex = (selectedTruckIndex + 1) % truckModels.size(); }
        }

        totalVehicles += config.vehicleCounts[type];
    }

    DrawText(TextFormat("TOTAL: %d vehicules", totalVehicles), 
             screenWidth/2 - 100, startY + vehicleTypes.size() * lineHeight + 20, 28, GOLD);
        
        // Boutons
        int btnY = startY + vehicleTypes.size() * lineHeight + 80;
        Rectangle btnStart = {(float)(screenWidth/2 - 250), (float)btnY, 200, 50};
        Rectangle btnReset = {(float)(screenWidth/2 + 50), (float)btnY, 200, 50};
        
        // Bouton Démarrer
        Color startColor = (totalVehicles > 0) ? GREEN : DARKGRAY;
        if (CheckCollisionPointRec(GetMousePosition(), btnStart) && totalVehicles > 0) {
            startColor = LIME;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                // Store selected model paths into config
                if (!carModels.empty()) config.selectedModelCar = carModels[std::clamp(selectedCarIndex, 0, (int)carModels.size()-1)];
                if (!busModels.empty()) config.selectedModelBus = busModels[std::clamp(selectedBusIndex, 0, (int)busModels.size()-1)];
                if (!truckModels.empty()) config.selectedModelTruck = truckModels[std::clamp(selectedTruckIndex, 0, (int)truckModels.size()-1)];

                // Print selection to console for debugging model loading
                std::cout << "Selected Models -> Car: '" << (config.selectedModelCar.empty() ? std::string("(none)") : config.selectedModelCar) 
                          << "', Bus: '" << (config.selectedModelBus.empty() ? std::string("(none)") : config.selectedModelBus) 
                          << "', Truck: '" << (config.selectedModelTruck.empty() ? std::string("(none)") : config.selectedModelTruck) << "'\n";

                config.isConfigured = true;
                startSimulation = true;
            }
        }
        DrawRectangleRec(btnStart, startColor);
        DrawRectangleLinesEx(btnStart, 3, RAYWHITE);
        DrawText("DEMARRER", 
                 btnStart.x + btnStart.width/2 - MeasureText("DEMARRER", 24)/2, 
                 btnStart.y + 13, 24, BLACK);
        
        // Bouton Reset
        Color resetColor = ORANGE;
        if (CheckCollisionPointRec(GetMousePosition(), btnReset)) {
            resetColor = Color{255, 200, 100, 255};
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                // Reset per-type counts
                for (const auto& t : vehicleTypes) config.vehicleCounts[t] = 0;
            }
        }
        DrawRectangleRec(btnReset, resetColor);
        DrawRectangleLinesEx(btnReset, 3, RAYWHITE);
        DrawText("RESET", 
                 btnReset.x + btnReset.width/2 - MeasureText("RESET", 24)/2, 
                 btnReset.y + 13, 24, BLACK);
        
        // Instructions
        DrawText("Cliquez et glissez sur les barres pour ajuster", 
                 screenWidth/2 - MeasureText("Cliquez et glissez sur les barres pour ajuster", 16)/2, 
                 screenHeight - 40, 16, LIGHTGRAY);
        
        EndDrawing();
    }
    
    CloseWindow();
    return config;
}

void LoadNetworkFlexible(RoadNetwork& network) {
    // 1. Try Loading from JSON
    std::cout << "Attempting to load configuration from: config/configuration.json" << std::endl;
    // Ensure accurate path (cwd is usually project root)
    if (MapLoader::LoadFromFile("config/configuration.json", network)) {
        std::cout << "SUCCESS: Network loaded from JSON. Nodes: " << network.GetNodes().size() << std::endl;
        if (network.GetNodes().empty()) {
             std::cerr << "WARNING: JSON loaded but empty! Falling back to hardcoded network." << std::endl;
             CreateTestNetwork(network);
        }
    } 
    else {
         // 2. Fallback
         std::cerr << "FAILURE: Could not load JSON config. Falling back to hardcoded network." << std::endl;
         CreateTestNetwork(network);
    }
}

int main() {
    SimulationConfig config = ShowConfigurationMenu();
    if (!config.isConfigured) return 0;
    
    // ==================== INITIALIZATION ====================
    InitWindow(1600, 900, "SMART CITY - Traffic Core Simulator");
    SetTargetFPS(60);
    
    std::vector<std::string> vehicleTypes = { "Car", "Bus", "Truck" };

    // Prepare model lists
    std::vector<std::string> carModels, busModels, truckModels;
    std::vector<std::string> candidateDirs = { "assets/models", "../assets/models", "../../assets/models", "../TrafficCore/assets/models", "TrafficCore/assets/models" };
    for (const auto& dir : candidateDirs) {
        try {
            if (!std::filesystem::exists(dir)) continue;
            for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                if (!entry.is_regular_file()) continue;
                auto path = entry.path();
                if (path.has_extension() && (path.extension() == ".glb" || path.extension() == ".GLB")) {
                    std::string lower = path.filename().string();
                    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                    if (lower.find("bus") != std::string::npos && lower.find("stop") == std::string::npos && lower.find("station") == std::string::npos) busModels.push_back(std::filesystem::absolute(path).string());
                    else if (lower.find("truck") != std::string::npos) truckModels.push_back(std::filesystem::absolute(path).string());
                    else if (lower.find("car") != std::string::npos || lower.find("taxi") != std::string::npos || lower.find("convertible") != std::string::npos) carModels.push_back(std::filesystem::absolute(path).string());
                }
            }
            break;
        } catch (...) {}
    }

    
    InitCamera();
    
    int sampleCountPerSegment = 12;
    TrafficManager trafficMgr;
    RoadNetwork network;
    LoadNetworkFlexible(network); // Uses JSON with fallback
    
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

    // PathFinder pour le calcul d'itinéraires
    PathFinder pathfinder(&network);
    
    // Charger modèles pour TrafficManager
    ModelManager& mm = ModelManager::getInstance();
    for (const auto& m : carModels) mm.loadModel("CAR", m);
    for (const auto& m : busModels) mm.loadModel("BUS", m);
    for (const auto& m : truckModels) mm.loadModel("TRUCK", m);
    // Spawner Setup (Entry at n1)
    const auto& nodes = network.GetNodes();
    // Link network to traffic manager so it can compute leader relationships and intersection logic
    trafficMgr.setRoadNetwork(&network);
    if (!nodes.empty()) {
        // Entry Points setup removed to enforce manual strict spawning from allowed nodes only.
    }


    // Spawn initial vehicles using ONLY the flux nodes (N1, N3, N7, N9, N10)
    if (!nodes.empty()) {
        // Identify flux nodes by position
        std::vector<int> fluxNodeIds;
        std::vector<Vector3> targetPos = {
            {1050.0f, 0.0f, -450.0f}, // N1
            {700.0f, 0.0f, -250.0f},  // N3
            {0.0f, 0.0f, -600.0f},    // N7
            {0.0f, 0.0f, 150.0f},     // N9
            {-150.0f, 0.0f, 0.0f}     // N10
        };
        
        for (const auto& n : nodes) {
            Vector3 p = n->GetPosition();
            for (const auto& t : targetPos) {
                if (fabs(p.x - t.x) < 5.0f && fabs(p.z - t.z) < 5.0f) {
                    fluxNodeIds.push_back(n->GetId());
                    break;
                }
            }
        }
        
        if (fluxNodeIds.size() >= 2) {
            static std::mt19937 rng((unsigned)time(nullptr));
            std::uniform_int_distribution<int> fluxDist(0, (int)fluxNodeIds.size() - 1);

            // Spawn Cars
            int carCount = config.vehicleCounts["Car"];
            for (int i = 0; i < carCount; ++i) {
                int s = fluxDist(rng);
                int e = fluxDist(rng);
                while (s == e && fluxNodeIds.size() > 1) e = fluxDist(rng);
                trafficMgr.spawnVehicleByNodeIds(fluxNodeIds[s], fluxNodeIds[e], VehiculeType::CAR);
            }
            // Spawn Buses
            int busCount = config.vehicleCounts["Bus"];
            for (int i = 0; i < busCount; ++i) {
                int s = fluxDist(rng);
                int e = fluxDist(rng);
                while (s == e && fluxNodeIds.size() > 1) e = fluxDist(rng);
                trafficMgr.spawnVehicleByNodeIds(fluxNodeIds[s], fluxNodeIds[e], VehiculeType::BUS);
            }
            // Spawn Trucks
            int truckCount = config.vehicleCounts["Truck"];
            for (int i = 0; i < truckCount; ++i) {
                int s = fluxDist(rng);
                int e = fluxDist(rng);
                while (s == e && fluxNodeIds.size() > 1) e = fluxDist(rng);
                trafficMgr.spawnVehicleByNodeIds(fluxNodeIds[s], fluxNodeIds[e], VehiculeType::TRUCK);
            }
        }
    }

    auto modelResolver = [&](VehiculeType t)->std::string {
        if (t == VehiculeType::CAR) return config.selectedModelCar;
        if (t == VehiculeType::BUS) return config.selectedModelBus;
        if (t == VehiculeType::TRUCK) return config.selectedModelTruck;
        return "";
    };

    // Itinerary resolver removed (Safety: prevent random spawning)

    int desiredVehicles = 0;
    for (auto const& [type, count] : config.vehicleCounts) desiredVehicles += count;

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
            0.2f,
            n5Pos.z + sinf(angle) * radius
        });
    }
    // Ajouter un au centre
    plantPositions.push_back({n5Pos.x, 0.2f, n5Pos.z});

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

    bool returnToMenu = false;

    Model buildingModel3 = LoadModel("../TrafficCore/assets/models/Large Building 3.glb");
    float building3Scale = 40.0f;
    std::vector<Vector3> building3Positions;
    // Row between n2 (x=700) and n4 (x=350)
    for (float x = 360.0f; x <= 700.0f; x += 45.0f) {
        building3Positions.push_back({ x, 0.0f, -520.0f });
    }

    // ==================== LOOP ====================
    bool paused = false;
    float simTime = 0.0f;
    while (!WindowShouldClose() && !returnToMenu) {
        float dt = GetFrameTime();
        if (!paused) simTime += dt;
        
        // Input
        if (IsKeyPressed(KEY_SPACE)) paused = !paused;
        
        // Retour au menu avec touche M
        if (IsKeyPressed(KEY_M)) {
            returnToMenu = true;
        }

        // Caméra
        UpdateCamera(trafficMgr, dt);

        // Ajouter véhicule (Touche V)
        if (IsKeyPressed(KEY_V)) {
            const auto& nodes = network.GetNodes();
            static std::vector<int> validSpawnIds;
            
            // Initialisation unique des 5 points de Flux (Entrée/Sortie)
            if (validSpawnIds.empty() && nodes.size() >= 2) {
                 std::vector<Vector3> targetPos = {
                     {1050.0f, 0.0f, -450.0f}, // N1
                     {700.0f, 0.0f, -250.0f},  // N3
                     {0.0f, 0.0f, -600.0f},    // N7
                     {0.0f, 0.0f, 150.0f},     // N9
                     {-150.0f, 0.0f, 0.0f}     // N10
                 };
                 
                 std::cout << "Identification des Noeuds de Flux (N1, N3, N7, N9, N10) :" << std::endl;
                 
                 for (const auto& n : nodes) {
                     Vector3 p = n->GetPosition();
                     for (const auto& t : targetPos) {
                         // Check X and Z with tolerance (ignore Y)
                         if (fabs(p.x - t.x) < 5.0f && fabs(p.z - t.z) < 5.0f) {
                             validSpawnIds.push_back(n->GetId());
                             std::cout << " - Flux Node Identified: ID " << n->GetId() << std::endl;
                             break;
                         }
                     }
                 }
            }

            if (!validSpawnIds.empty()) {
                static std::mt19937 rng((unsigned)time(nullptr));
                
                // 1. Choisir un point de départ parmi les points de flux
                std::uniform_int_distribution<int> dist(0, (int)validSpawnIds.size() - 1);
                int startId = validSpawnIds[dist(rng)];
                
                // 2. Choisir un point d'arrivée (obligatoirement un point de flux aussi)
                int endId = validSpawnIds[dist(rng)];
                
                // Éviter start == end
                int attempts = 0;
                while (endId == startId && attempts < 10) {
                    endId = validSpawnIds[dist(rng)];
                    attempts++;
                }
                
                // Random vehicle type
                std::uniform_int_distribution<int> tdist(0,2);
                VehiculeType vt = static_cast<VehiculeType>(tdist(rng));
                
                bool ok = trafficMgr.spawnVehicleByNodeIds(startId, endId, vt);
                if (ok) std::cout << "Spawned vehicle from Node " << startId << " to " << endId << std::endl;
            }
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
            trafficMgr.removeFinishedVehicles();
            
            // Spawn automatique désactivé : Utiliser la touche V pour ajouter des véhicules
            // (garantit que seuls les nœuds de flux N1, N3, N7, N9, N10 sont utilisés)
            /*
            if (trafficMgr.getVehicleCount() < desiredVehicles && trafficMgr.hasPending()) {
                trafficMgr.spawnNext(modelResolver, itineraryResolver);
            }
            */
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
        

        
        DrawUIPanel(10, 10, 350, 180, "SMART CITY - BOUKHALF 2030");
        DrawText(TextFormat("Vehicles: %d", trafficMgr.getVehicleCount()), 20, 50, 18, GREEN);
        DrawText(TextFormat("Time: %.1f s", simTime), 20, 75, 16, SKYBLUE);
        DrawText(paused ? "PAUSED" : "RUNNING", 20, 100, 16, paused ? RED : GREEN);
        DrawCameraInfo();
        
        DrawUIPanel(10, 200, 350, 340, "CONTROLES");
        DrawText("SPACE     : Pause/Resume", 20, 225, 13, WHITE);
        DrawText("V / K     : Add / Delete Vehicle", 20, 243, 13, WHITE);
        DrawText("M         : Configuration Menu", 20, 261, 13, ORANGE);
        DrawText("R         : Reset Camera", 20, 279, 13, WHITE);
        DrawText("C         : Cinematic Mode", 20, 297, 13, SKYBLUE);
        DrawText("1 / 2 / 3 : Camera Modes", 20, 328, 13, YELLOW);
        DrawText("WASD/ZQSD : Move", 20, 403, 12, GREEN);
        
        DrawFPS(1450, 870);
        EndDrawing();
    }
    
    EnableCursor();
    CloseWindow();
    
    // Si l'utilisateur a appuyé sur M, recommencer avec un nouveau menu
    if (returnToMenu) {
        return main(); // Relancer le programme (récursif)
    }
    
    return 0;
}


// ==================== UI ====================
void DrawUIPanel(int x, int y, int width, int height, const char* title) {
    DrawRectangle(x, y, width, height, Fade(BLACK, 0.8f));
    DrawRectangleLines(x, y, width, height, YELLOW);
    DrawText(title, x + 10, y + 8, 16, YELLOW);
}

// Try loading network from configuration.json; fallback to CreateTestNetwork
void InitializeNetworkFromConfig(RoadNetwork& network) {
    const std::string cfgPath = "TrafficCore/config/configuration.json";
    bool ok = false;
    try {
        ok = MapLoader::LoadFromFile(cfgPath, network);
    } catch (...) { ok = false; }
    if (!ok) {
        CreateTestNetwork(network);
    }
}