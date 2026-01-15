#include "Vehicules/EmergencyVehicle.h"
#include "Vehicules/Vehicule.h"
#include "PathFinder.h"
#include "RoadNetwork.h"
#include "Node.h"
#include "RoadSegment.h"
#include <deque>
#include "rlgl.h"
#include <algorithm>

EmergencyVehicle::EmergencyVehicle(Vector3 pos, EmergencyType type, Model model, RoadNetwork* network)
    : Vehicule(pos, 140.0f, 10.0f, model, 1.2f, RED), 
      emergencyType(type),
      network(network),
      isOnEmergencyMission(false),
      destinationNode(nullptr)
{
    // Vitesse supérieure aux voitures normales (100.0f dans Car.cpp)
    this->maxSpeed = 140.0f; 
    this->acceleration = 10.0f;
    this->isSirenActive = true;
    
    // Couleur selon le type
    switch (type) {
        case AMBULANCE:
            debugColor = WHITE; // Distinct from Fire Truck
            break;
        case FIRE_TRUCK:
            debugColor = Color{255, 140, 0, 255}; // Orange
            break;
        case POLICE:
            debugColor = BLUE;
            break;
    }
}

void EmergencyVehicle::update(float deltaTime) {
    sirenTimer += deltaTime;
    
    // If not on mission, stay parked (do not move)
    if (!isOnEmergencyMission) return;
    
    // Si en mission d'urgence, ignorer les feux rouges
    if (isOnEmergencyMission) {
        // Les feux sont gérés par EmergencyManager
    }
    
    // Logique de physique parente
    updatePhysics(deltaTime);
}

void EmergencyVehicle::draw() {
    if (model.meshCount > 0) {
        // Rendu du modèle 3D externe si disponible
        Vehicule::draw();
        
        // Effet visuel de gyrophare simple pour le modèle
        if (isSirenActive) {
            Color lightColor1 = ((int)(sirenTimer * 10) % 2 == 0) ? RED : BLUE;
            Color lightColor2 = ((int)(sirenTimer * 10) % 2 == 0) ? BLUE : RED;
            Vector3 lightPos1 = { position.x - 1.0f, position.y + 2.5f, position.z };
            Vector3 lightPos2 = { position.x + 1.0f, position.y + 2.5f, position.z };
            DrawSphere(lightPos1, 0.6f, lightColor1);
            DrawSphere(lightPos2, 0.6f, lightColor2);
        }
    } else {
        // Rendu Procédural Réaliste (si pas de modèle)
        rlPushMatrix();
        rlTranslatef(position.x, position.y, position.z);
        rlRotatef(getRotationAngle(), 0, 1, 0); // Rotation locale

        switch (emergencyType) {
            case AMBULANCE: drawAmbulance(); break;
            case POLICE: drawPoliceCar(); break;
            case FIRE_TRUCK: drawFireTruck(); break;
        }

        rlPopMatrix();
    }
}

void EmergencyVehicle::drawSiren(Vector3 localPos) {
    // Base du gyrophare
    DrawCube(localPos, 1.2f, 0.1f, 0.3f, DARKGRAY);
    
    if (isSirenActive) {
        Color c1 = ((int)(sirenTimer * 12) % 2 == 0) ? RED : Fade(RED, 0.2f);
        Color c2 = ((int)(sirenTimer * 12) % 2 == 0) ? Fade(BLUE, 0.2f) : BLUE;
        
        // Lumières clignotantes
        DrawCube({localPos.x - 0.4f, localPos.y + 0.15f, localPos.z}, 0.3f, 0.3f, 0.3f, c1);
        DrawCube({localPos.x + 0.4f, localPos.y + 0.15f, localPos.z}, 0.3f, 0.3f, 0.3f, c2);
        
        // Halo lumineux
        DrawSphere({localPos.x - 0.4f, localPos.y + 0.15f, localPos.z}, 0.6f, Fade(c1, 0.3f));
        DrawSphere({localPos.x + 0.4f, localPos.y + 0.15f, localPos.z}, 0.6f, Fade(c2, 0.3f));
    } else {
        // Éteint
        DrawCube({localPos.x - 0.4f, localPos.y + 0.15f, localPos.z}, 0.3f, 0.3f, 0.3f, MAROON);
        DrawCube({localPos.x + 0.4f, localPos.y + 0.15f, localPos.z}, 0.3f, 0.3f, 0.3f, DARKBLUE);
    }
}

void EmergencyVehicle::drawAmbulance() {
    // --- AMBULANCE (Type Box) ---
    // Châssis principal (Arrière)
    DrawCube({0.0f, 1.6f, -0.5f}, 4.0f, 2.2f, 4.0f, WHITE);
    // Cabine (Avant)
    DrawCube({0.0f, 1.2f, 2.2f}, 4.0f, 1.4f, 1.6f, WHITE);
    
    // Bandes Rouges
    DrawCube({0.0f, 1.6f, -0.5f}, 4.05f, 0.4f, 4.1f, RED); // Bande latérale
    DrawCube({0.0f, 0.8f, 2.2f}, 4.05f, 0.6f, 1.65f, WHITE); // Pare-chocs avant
    
    // Croix Médicale (Sur les flancs)
    // Gauche
    DrawCube({2.05f, 1.8f, -0.5f}, 0.1f, 0.8f, 0.25f, RED);
    DrawCube({2.05f, 1.8f, -0.5f}, 0.1f, 0.25f, 0.8f, RED);
    // Droite
    DrawCube({-2.05f, 1.8f, -0.5f}, 0.1f, 0.8f, 0.25f, RED);
    DrawCube({-2.05f, 1.8f, -0.5f}, 0.1f, 0.25f, 0.8f, RED);
    
    // Vitres (Cabine)
    DrawCube({0.0f, 1.5f, 2.5f}, 3.9f, 0.6f, 1.05f, DARKGRAY); // Pare-brise
    
    // Roues
    DrawCylinder({1.9f, 0.5f, 1.8f}, 0.5f, 0.5f, 0.4f, 10, BLACK); // AV G
    DrawCylinder({-1.9f, 0.5f, 1.8f}, 0.5f, 0.5f, 0.4f, 10, BLACK); // AV D
    DrawCylinder({1.9f, 0.5f, -1.5f}, 0.5f, 0.5f, 0.4f, 10, BLACK); // AR G
    DrawCylinder({-1.9f, 0.5f, -1.5f}, 0.5f, 0.5f, 0.4f, 10, BLACK); // AR D
    
    // Gyrophare
    drawSiren({0.0f, 2.75f, 2.0f});
}

void EmergencyVehicle::drawPoliceCar() {
    // --- POLICE (Sedan Sportive) ---
    // Corps bas
    DrawCube({0.0f, 0.7f, 0.0f}, 3.2f, 0.8f, 4.8f, DARKBLUE);
    // Habitacle (Haut)
    DrawCube({0.0f, 1.3f, -0.2f}, 2.9f, 0.7f, 2.5f, WHITE);
    
    // Portes Blanches (Contraste Police)
    DrawCube({0.0f, 0.7f, 0.0f}, 3.25f, 0.75f, 2.0f, WHITE);
    
    // Vitres
    DrawCube({0.0f, 1.35f, -0.2f}, 2.8f, 0.55f, 2.6f, BLACK); // Vitres teintées
    
    // Capot et Coffre (Détails)
    DrawCube({0.0f, 0.75f, 1.8f}, 3.0f, 0.1f, 1.2f, DARKBLUE); // Capot
    DrawCube({0.0f, 0.75f, -2.0f}, 3.0f, 0.1f, 0.8f, DARKBLUE); // Coffre
    
    // Pare-chocs
    DrawCube({0.0f, 0.4f, 2.4f}, 3.2f, 0.4f, 0.2f, BLACK); // Avant
    DrawCube({0.0f, 0.4f, -2.4f}, 3.2f, 0.4f, 0.2f, BLACK); // Arrière
    
    // Roues
    float wheelZ = 1.6f;
    DrawCylinder({1.5f, 0.4f, wheelZ}, 0.4f, 0.4f, 0.3f, 10, BLACK);
    DrawCylinder({-1.5f, 0.4f, wheelZ}, 0.4f, 0.4f, 0.3f, 10, BLACK);
    DrawCylinder({1.5f, 0.4f, -wheelZ}, 0.4f, 0.4f, 0.3f, 10, BLACK);
    DrawCylinder({-1.5f, 0.4f, -wheelZ}, 0.4f, 0.4f, 0.3f, 10, BLACK);
    
    // Gyrophare (Barre de toit)
    drawSiren({0.0f, 1.7f, -0.2f});
}

void EmergencyVehicle::drawFireTruck() {
    // --- FIRE TRUCK (Camion Pompier) ---
    // Corps Principal (Grand réservoir)
    DrawCube({0.0f, 1.8f, -1.0f}, 4.5f, 2.2f, 5.0f, RED);
    
    // Cabine Avant
    DrawCube({0.0f, 1.5f, 2.5f}, 4.5f, 1.8f, 1.8f, RED);
    
    // Détails Blancs (Bandes)
    DrawCube({0.0f, 1.0f, 0.0f}, 4.55f, 0.4f, 7.0f, WHITE);
    
    // Grande Échelle (Sur le toit)
    // Base échelle
    DrawCube({0.0f, 3.0f, -0.5f}, 1.4f, 0.2f, 5.0f, LIGHTGRAY);
    // Barreaux (Simulation visuelle)
    for(int i=0; i<5; i++) {
        DrawCube({0.0f, 3.2f, -2.0f + i*1.0f}, 1.6f, 0.1f, 0.1f, DARKGRAY);
    }
    // Vérins hydrauliques (Déco)
    DrawCylinder({1.2f, 3.0f, -2.0f}, 0.1f, 0.1f, 2.0f, 6, GRAY);
    DrawCylinder({-1.2f, 3.0f, -2.0f}, 0.1f, 0.1f, 2.0f, 6, GRAY);
    
    // Vitres Cabine
    DrawCube({0.0f, 1.8f, 2.8f}, 4.4f, 0.8f, 1.3f, BLACK);
    
    // Roues (6 roues pour camion)
    float wheelY = 0.6f;
    float wheelR = 0.6f;
    // Avant
    DrawCylinder({2.15f, wheelY, 2.5f}, wheelR, wheelR, 0.5f, 12, BLACK);
    DrawCylinder({-2.15f, wheelY, 2.5f}, wheelR, wheelR, 0.5f, 12, BLACK);
    // Arrière 1
    DrawCylinder({2.15f, wheelY, -1.5f}, wheelR, wheelR, 0.5f, 12, BLACK);
    DrawCylinder({-2.15f, wheelY, -1.5f}, wheelR, wheelR, 0.5f, 12, BLACK);
    // Arrière 2
    DrawCylinder({2.15f, wheelY, -2.8f}, wheelR, wheelR, 0.5f, 12, BLACK);
    DrawCylinder({-2.15f, wheelY, -2.8f}, wheelR, wheelR, 0.5f, 12, BLACK);
    
    // Gyrophares (Multiples)
    drawSiren({1.8f, 2.5f, 2.8f});
    drawSiren({-1.8f, 2.5f, 2.8f});
}

void EmergencyVehicle::setEmergencyMission(Node* destination) {
    if (!network || !destination) return;
    
    destinationNode = destination;
    isOnEmergencyMission = true;
    isSirenActive = true;
    
    // Calculer le chemin le plus court vers la destination
    Node* startNode = findNearestNode();
    if (!startNode) return;
    
    PathFinder pathfinder(network);
    std::vector<Node*> nodePath = pathfinder.FindPath(startNode, destination);
    
    if (nodePath.empty()) {
        isOnEmergencyMission = false;
        return;
    }
    
    // Convertir le chemin de noeuds en chemin de segments routiers
    std::deque<RoadSegment*> roadRoute;
    for (size_t i = 0; i < nodePath.size() - 1; ++i) {
        Node* u = nodePath[i];
        Node* v = nodePath[i+1];
        
        RoadSegment* connecting = nullptr;
        const auto& allSegments = network->GetRoadSegments();
        for (const auto& segPtr : allSegments) {
            if (segPtr->GetStartNode() == u && segPtr->GetEndNode() == v) {
                connecting = segPtr.get();
                break;
            }
        }
        if (connecting) {
            roadRoute.push_back(connecting);
        }
    }
    
    if (!roadRoute.empty()) {
        // Utiliser la voie la plus à gauche pour dépasser plus facilement
        RoadSegment* firstRoad = roadRoute.front();
        int forwardLanes = firstRoad->GetLanes() / 2;
        if (forwardLanes > 0) {
            setLane(forwardLanes - 1); // Voie la plus à gauche
        }
        setRoute(roadRoute);
    }
}

Node* EmergencyVehicle::findNearestNode() const {
    if (!network) return nullptr;
    
    Node* nearest = nullptr;
    float minDist = 999999.0f;
    
    for (const auto& node : network->GetNodes()) {
        float dist = Vector3Distance(position, node->GetPosition());
        if (dist < minDist) {
            minDist = dist;
            nearest = node.get();
        }
    }
    
    return nearest;
}

bool EmergencyVehicle::hasReachedDestination() const {
    if (!destinationNode || !isOnEmergencyMission) return false;
    
    float dist = Vector3Distance(position, destinationNode->GetPosition());
    return dist < 15.0f; // Distance de tolérance pour l'arrivée
}

void EmergencyVehicle::completeMission() {
    isOnEmergencyMission = false;
    isSirenActive = false;
    destinationNode = nullptr;
}

EmergencyType EmergencyVehicle::getType() const {
    return emergencyType;
}
