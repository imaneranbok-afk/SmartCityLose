#include "RoadNetwork.h"
#include <iostream>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <limits>
#include <cmath>
#include <algorithm>
#include <functional>
#include "raymath.h"

RoadNetwork::RoadNetwork() : nextNodeId(1) {}

RoadNetwork::~RoadNetwork() {
    Clear();
}

Node* RoadNetwork::AddNode(Vector3 position, NodeType type, float radius) {
    auto node = std::make_unique<Node>(nextNodeId++, position, type, radius);
    Node* nodePtr = node.get();
    nodes.push_back(std::move(node));
    return nodePtr;
}

RoadSegment* RoadNetwork::AddRoadSegment(Node* start, Node* end, int lanes, bool curved) {
    if (!start || !end) {
        std::cerr << "Erreur: Tentative d'ajout d'un segment avec des noeuds null" << std::endl;
        return nullptr;
    }
    
    auto segment = std::make_unique<RoadSegment>(start, end, lanes, curved);
    RoadSegment* segmentPtr = segment.get();
    roadSegments.push_back(std::move(segment));
    return segmentPtr;
}

Intersection* RoadNetwork::AddIntersection(Node* node) {
    if (!node) {
        std::cerr << "Erreur: Tentative d'ajout d'une intersection avec un noeud null" << std::endl;
        return nullptr;
    }
    
    auto intersection = std::make_unique<Intersection>(node);
    Intersection* intersectionPtr = intersection.get();
    intersections.push_back(std::move(intersection));
    return intersectionPtr;
}

Node* RoadNetwork::FindNodeById(int id) const {
    for (const auto& node : nodes) {
        if (node->GetId() == id) {
            return node.get();
        }
    }
    return nullptr;
}

// RoadNetwork no longer contains pathfinding logic; use PathFinder class instead.

void RoadNetwork::Update(float deltaTime) {
    for (const auto& intersection : intersections) {
        intersection->Update(deltaTime);
    }
}

void RoadNetwork::Draw() const {
    // Dessiner d'abord toutes les routes
    for (const auto& segment : roadSegments) {
        segment->Draw();
    }
    
    // Puis les intersections (par-dessus pour masquer les raccords)
    for (const auto& intersection : intersections) {
        intersection->Draw();
    }
}

float RoadNetwork::GetTotalRoadLength() const {
    float totalLength = 0.0f;
    for (const auto& segment : roadSegments) {
        totalLength += segment->GetLength();
    }
    return totalLength;
}

void RoadNetwork::Clear() {
    intersections.clear();
    roadSegments.clear();
    nodes.clear();
    nextNodeId = 1;
}

void RoadNetwork::PrintNetworkInfo() const {
    std::cout << "\n=== Informations sur le réseau routier ===" << std::endl;
    std::cout << "Nombre de noeuds: " << GetNodeCount() << std::endl;
    std::cout << "Nombre de segments: " << GetRoadSegmentCount() << std::endl;
    std::cout << "Nombre d'intersections: " << GetIntersectionCount() << std::endl;
    std::cout << "Longueur totale des routes: " << GetTotalRoadLength() << " unités" << std::endl;
    
    // Détails des noeuds
    std::cout << "\n--- Noeuds ---" << std::endl;
    for (const auto& node : nodes) {
        std::string typeStr;
        switch (node->GetType()) {
            case SIMPLE_INTERSECTION: typeStr = "Intersection Simple"; break;
            case ROUNDABOUT: typeStr = "Rond-Point"; break;
            case TRAFFIC_LIGHT: typeStr = "Feux Tricolores"; break;
        }
        Vector3 pos = node->GetPosition();
        std::cout << "Noeud #" << node->GetId() 
                  << " (" << typeStr << ") - Position: (" 
                  << pos.x << ", " << pos.y << ", " << pos.z << ")" 
                  << " - Rayon: " << node->GetRadius() << std::endl;
    }
}