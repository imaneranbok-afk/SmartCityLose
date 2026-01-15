#include "Node.h"
#include "RoadSegment.h"
#include "raymath.h"

Node::Node(int id, Vector3 position, NodeType type, float radius)
    : id(id), position(position), type(type), radius(radius),
      lightState(LIGHT_GREEN), lightTimer(0.0f),
      redDuration(5.0f), yellowDuration(2.0f), greenDuration(8.0f),
      emergencyOverride(false), emergencyOverrideTimer(0.0f) {}

void Node::AddConnectedRoad(RoadSegment* road) {
    connectedRoads.push_back(road);
}

Vector3 Node::GetConnectionTangent(Vector3 direction) const {
    direction = Vector3Normalize(direction);
    
    if (type == ROUNDABOUT) {
        // Pour un rond-point, la tangente est perpendiculaire au rayon
        Vector3 toCenter = Vector3Subtract(position, Vector3Add(position, Vector3Scale(direction, radius)));
        return Vector3CrossProduct(toCenter, {0, 1, 0});
    }
    
    // Pour les intersections simples, la direction reste la même
    return direction;
}

void Node::UpdateTrafficLight(float deltaTime) {
    if (type != TRAFFIC_LIGHT) return;
    
    // Gestion du override d'urgence
    if (emergencyOverride) {
        emergencyOverrideTimer -= deltaTime;
        if (emergencyOverrideTimer <= 0.0f) {
            emergencyOverride = false;
            lightTimer = 0.0f; // Reset pour reprendre le cycle normal
        }
        return; // Pendant l'override, on reste au vert
    }
    
    // Cycle normal des feux
    lightTimer += deltaTime;
    
    switch (lightState) {
        case LIGHT_GREEN:
            if (lightTimer >= greenDuration) {
                lightState = LIGHT_YELLOW;
                lightTimer = 0.0f;
            }
            break;
        case LIGHT_YELLOW:
            if (lightTimer >= yellowDuration) {
                lightState = LIGHT_RED;
                lightTimer = 0.0f;
            }
            break;
        case LIGHT_RED:
            if (lightTimer >= redDuration) {
                lightState = LIGHT_GREEN;
                lightTimer = 0.0f;
            }
            break;
    }
}

void Node::SetEmergencyOverride(bool override, float duration) {
    emergencyOverride = override;
    emergencyOverrideTimer = duration;
    if (override) {
        lightState = LIGHT_GREEN; // Force au vert immédiatement
    }
}

void Node::Draw() const {
    Color roadColor = {50, 50, 50, 255};
    Color lineColor = {255, 255, 255, 255};
    
    switch (type) {
        case ROUNDABOUT:
            // Le rond-point est dessiné via RoundaboutGeometry dans Intersection
            break;
            
        case TRAFFIC_LIGHT: {
            // Calculer la taille basée sur les routes connectées
            float maxRoadWidth = 0.0f;
            for (const auto* road : connectedRoads) {
                float roadWidth = road->GetWidth();
                if (roadWidth > maxRoadWidth) maxRoadWidth = roadWidth;
            }
            
            // Taille de l'intersection = max(radius, largeur route maximale)
            float intersectionSize = fmaxf(radius, maxRoadWidth * 0.5f);
            
            // Zone d'intersection carrée
            DrawCube(position, intersectionSize * 2, 0.01f, intersectionSize * 2, roadColor);
            
            // Feux tricolores aux 4 coins
            Vector3 corners[4] = {
                {position.x + intersectionSize, position.y, position.z + intersectionSize},
                {position.x + intersectionSize, position.y, position.z - intersectionSize},
                {position.x - intersectionSize, position.y, position.z + intersectionSize},
                {position.x - intersectionSize, position.y, position.z - intersectionSize}
            };
            
            // Déterminer la couleur active selon l'état
            Color activeRed = (lightState == LIGHT_RED || emergencyOverride) ? RED : Fade(RED, 0.3f);
            Color activeYellow = (lightState == LIGHT_YELLOW && !emergencyOverride) ? YELLOW : Fade(YELLOW, 0.3f);
            Color activeGreen = (lightState == LIGHT_GREEN || emergencyOverride) ? GREEN : Fade(GREEN, 0.3f);
            
            for (int i = 0; i < 4; i++) {
                // Poteau
                DrawCylinder(corners[i], 1.0f, 1.0f, 13.0f, 8, DARKGRAY); 
                
                // Configuration de l'orientation selon le coin pour qu'ils se fassent face
                Vector3 boxSize = {2.2f, 6.0f, 1.6f}; // Par défaut orienté Z
                Vector3 lightOffset = {0.0f, 0.0f, 0.85f}; // Par défaut face +Z
                
                if (i == 0) { // Coin (+X, +Z) -> Face +Z
                    boxSize = {2.2f, 6.0f, 1.6f};
                    lightOffset = {0.0f, 0.0f, 0.85f};
                } 
                else if (i == 3) { // Coin (-X, -Z) -> Face -Z
                    boxSize = {2.2f, 6.0f, 1.6f};
                    lightOffset = {0.0f, 0.0f, -0.85f};
                }
                else if (i == 1) { // Coin (+X, -Z) -> Face +X
                    boxSize = {1.6f, 6.0f, 2.2f}; // Échange largeur/profondeur
                    lightOffset = {0.85f, 0.0f, 0.0f};
                }
                else if (i == 2) { // Coin (-X, +Z) -> Face -X
                    boxSize = {1.6f, 6.0f, 2.2f}; // Échange largeur/profondeur
                    lightOffset = {-0.85f, 0.0f, 0.0f};
                }

                // Boîtier du feu
                Vector3 lightPos = {corners[i].x, corners[i].y + 10.0f, corners[i].z};
                DrawCube(lightPos, boxSize.x, boxSize.y, boxSize.z, BLACK);
                
                // Feux (rouge, jaune, vert) avec état actif
                DrawSphere({lightPos.x + lightOffset.x, lightPos.y + 1.8f, lightPos.z + lightOffset.z}, 0.9f, activeRed);
                DrawSphere({lightPos.x + lightOffset.x, lightPos.y, lightPos.z + lightOffset.z}, 0.9f, activeYellow);
                DrawSphere({lightPos.x + lightOffset.x, lightPos.y - 1.8f, lightPos.z + lightOffset.z}, 0.9f, activeGreen);
            }
            
            break;
        }
        
        case SIMPLE_INTERSECTION:
        default: {
            // Calculer la taille basée sur les routes connectées
            float maxRoadWidth = 0.0f;
            for (const auto* road : connectedRoads) {
                float roadWidth = road->GetWidth();
                if (roadWidth > maxRoadWidth) maxRoadWidth = roadWidth;
            }
            
            // Taille de l'intersection = max(radius, largeur route maximale)
            float intersectionSize = fmaxf(radius, maxRoadWidth * 0.5f);
            
            // Intersection simple : carré gris
            DrawCube(position, intersectionSize * 2, 0.01f, intersectionSize * 2, roadColor);
            
            
            break;
        }
    }
}