#include "Node.h"
#include "RoadSegment.h"
#include "raymath.h"

Node::Node(int id, Vector3 position, NodeType type, float radius)
    : id(id), position(position), type(type), radius(radius) {}

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
            
            for (int i = 0; i < 4; i++) {
                // Poteau
                DrawCylinder(corners[i], 0.4f, 0.4f, 6.0f, 8, DARKGRAY); 
                
                // Boîtier du feu
                Vector3 lightPos = {corners[i].x, corners[i].y + 5.0f, corners[i].z};
                DrawCube(lightPos, 0.6f, 1.5f, 0.4f, BLACK);
                
                // Feux (rouge, jaune, vert)
                DrawSphere({lightPos.x, lightPos.y + 0.5f, lightPos.z + 0.25f}, 0.22f, RED);
                DrawSphere({lightPos.x, lightPos.y, lightPos.z + 0.25f}, 0.22f, YELLOW);
                DrawSphere({lightPos.x, lightPos.y - 0.5f, lightPos.z + 0.25f}, 0.22f, GREEN);
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