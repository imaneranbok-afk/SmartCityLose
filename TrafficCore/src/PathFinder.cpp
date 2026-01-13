#include "PathFinder.h"
#include "RoadNetwork.h"
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include "raymath.h"

struct PQItem {
    Node* node;
    float f;
    bool operator>(const PQItem& o) const { return f > o.f; }
};

PathFinder::PathFinder(const RoadNetwork* network) : network(network) {}

// Coût d'une arête (RoadSegment) : longueur / (1 + k * lanes)
static float EdgeCost(RoadSegment* seg) {
    if (!seg) return 1e6f;
    float len = seg->GetLength();
    int lanes = seg->GetLanes();
    float laneFactor = 1.0f / (1.0f + 0.18f * static_cast<float>(lanes));
    return len * laneFactor;
}

std::vector<Node*> PathFinder::FindPath(Node* start, Node* end) const {
    if (!network || !start || !end) return {};
    if (start == end) return {start};

    auto heuristic = [](Node* a, Node* b) {
        Vector3 pa = a->GetPosition();
        Vector3 pb = b->GetPosition();
        return Vector3Distance(pa, pb);
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

        for (RoadSegment* seg : current->GetConnectedRoads()) {
            if (!seg) continue;
            Node* neighbor = (seg->GetStartNode() == current) ? seg->GetEndNode() : seg->GetStartNode();
            if (!neighbor) continue;
            if (closedSet.find(neighbor) != closedSet.end()) continue;

            float tentative_g = gScore[current] + EdgeCost(seg);

            auto itg = gScore.find(neighbor);
            if (itg == gScore.end() || tentative_g < itg->second) {
                cameFrom[neighbor] = current;
                gScore[neighbor] = tentative_g;
                float f = tentative_g + heuristic(neighbor, end);
                openQueue.push(PQItem{neighbor, f});
            }
        }
    }

    return {};
}
