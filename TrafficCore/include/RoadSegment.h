#ifndef ROADSEGMENT_H
#define ROADSEGMENT_H

#include "Node.h"
#include "geometry/RoadGeometryStrategy.h"
#include <memory>
#include <vector>

class RoadSegment {
private:
    Node* startNode;
    Node* endNode;
    int lanes;
    float laneWidth;
    std::unique_ptr<RoadGeometryStrategy> geometry;
    bool visible = true; // Default to true

public:
    struct Sidewalk {
        std::vector<Vector3> path;
        float width;
        float height;
    };
    const std::vector<Sidewalk>& GetSidewalks() const { return sidewalks; }

private:
    std::vector<Sidewalk> sidewalks;

    void CreateGeometry(bool useCurvedConnection);
    void CreateSidewalks();
    void DrawSidewalk(const Sidewalk& sidewalk) const;
    void DrawCrosswalk(Vector3 position, Vector3 direction, float roadWidth) const;  // AJOUTÉ
    
    float CalculateIntersectionClearance(Node* node) const;

public:
    RoadSegment(Node* start, Node* end, int lanes, bool useCurvedConnection = true);

    void Draw() const;
    void SetVisible(bool v) { visible = v; }
    bool IsVisible() const { return visible; }

    Node* GetStartNode() const { return startNode; }
    Node* GetEndNode() const { return endNode; }
    int GetLanes() const { return lanes; }
    float GetWidth() const { return lanes * laneWidth; }
    float GetLength() const;
    RoadGeometryStrategy* GetGeometry() const { return geometry.get(); }

    Vector3 GetLanePosition(int laneIndex, float t) const;

    // Calcule la position sur une voie spécifique (0-1: Aller, 2-3: Retour)
    // t: progression 0.0 à 1.0 le long du segment
    Vector3 GetTrafficLanePosition(int laneIndex, float t) const;
    
    // Accesseurs
    Vector3 GetStartPos() const { return startNode->GetPosition(); }
    Vector3 GetEndPos() const { return endNode->GetPosition(); }
    Vector3 GetDirection() const;
    
    // Retourne la progression le long du segment pour une position donnée (0..1),
    // ou -1 si la position est trop éloignée du segment.
    float ComputeProgressOnSegment(const Vector3& pos) const;
};

#endif