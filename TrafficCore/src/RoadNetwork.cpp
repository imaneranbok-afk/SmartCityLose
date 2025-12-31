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

std::vector<Node*> RoadNetwork::FindPath(Node* start, Node* end) const {
    if (!start || !end) return {};
    if (start == end) return {start};

    auto heuristic = [](Node* a, Node* b) {
        Vector3 pa = a->GetPosition();
        Vector3 pb = b->GetPosition();
        return Vector3Distance(pa, pb);
    };

    struct PQItem {
        Node* node;
        float f;
        bool operator>(const PQItem& o) const { return f > o.f; }
    };

    std::priority_queue<PQItem, std::vector<PQItem>, std::greater<PQItem>> openQueue;
    std::unordered_map<Node*, Node*> cameFrom;
    std::unordered_map<Node*, float> gScore;
    std::unordered_set<Node*> closedSet;

    gScore[start] = 0.0f;
    openQueue.push(PQItem{start, heuristic(start, end)});

    while (!openQueue.empty()) {
        Node* current = openQueue.top().node;
        openQueue.pop();

        if (current == end) {
            // Reconstruct path
            std::vector<Node*> path;
            Node* it = end;
            while (it) {
                path.push_back(it);
                auto found = cameFrom.find(it);
                if (found == cameFrom.end()) break;
                it = found->second;
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        if (closedSet.find(current) != closedSet.end()) continue;
        closedSet.insert(current);

        // iterate neighbors via connected roads
        for (RoadSegment* seg : current->GetConnectedRoads()) {
            if (!seg) continue;
            Node* neighbor = (seg->GetStartNode() == current) ? seg->GetEndNode() : seg->GetStartNode();
            if (!neighbor) continue;

            if (closedSet.find(neighbor) != closedSet.end()) continue;

            float tentative_g = gScore[current] + Vector3Distance(current->GetPosition(), neighbor->GetPosition());

            auto itg = gScore.find(neighbor);
            if (itg == gScore.end() || tentative_g < itg->second) {
                cameFrom[neighbor] = current;
                gScore[neighbor] = tentative_g;
                float f = tentative_g + heuristic(neighbor, end);
                openQueue.push(PQItem{neighbor, f});
            }
        }
    }

    // No path found
    return {};
}

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