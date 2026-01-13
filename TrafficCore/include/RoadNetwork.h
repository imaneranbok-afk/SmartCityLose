#ifndef ROADNETWORK_H
#define ROADNETWORK_H

#include "Node.h"
#include "RoadSegment.h"
#include "Intersection.h"
#include <vector>
#include <memory>
#include <string>

class RoadNetwork {
private:
    std::vector<std::unique_ptr<Node>> nodes;
    std::vector<std::unique_ptr<RoadSegment>> roadSegments;
    std::vector<std::unique_ptr<Intersection>> intersections;
    
    int nextNodeId;
    
public:
    RoadNetwork();
    ~RoadNetwork();
    
    // Construction du réseau
    Node* AddNode(Vector3 position, NodeType type = SIMPLE_INTERSECTION, float radius = 5.0f);
    RoadSegment* AddRoadSegment(Node* start, Node* end, int lanes, bool curved = true);
    Intersection* AddIntersection(Node* node);
    
    // Accès aux éléments
    const std::vector<std::unique_ptr<Node>>& GetNodes() const { return nodes; }
    const std::vector<std::unique_ptr<RoadSegment>>& GetRoadSegments() const { return roadSegments; }
    const std::vector<std::unique_ptr<Intersection>>& GetIntersections() const { return intersections; }
    
    // Trouver un noeud par ID
    Node* FindNodeById(int id) const;
    
    // Pathfinding is now handled by PathFinder class.
    
    // Mise à jour et rendu
    void Update(float deltaTime);
    void Draw() const;
    
    // Statistiques
    int GetNodeCount() const { return static_cast<int>(nodes.size()); }
    int GetRoadSegmentCount() const { return static_cast<int>(roadSegments.size()); }
    int GetIntersectionCount() const { return static_cast<int>(intersections.size()); }
    float GetTotalRoadLength() const;
    
    // Utilitaires
    void Clear();
    void PrintNetworkInfo() const;
};

#endif