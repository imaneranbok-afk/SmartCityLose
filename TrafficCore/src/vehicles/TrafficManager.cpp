#include "Vehicules/TrafficManager.h"
#include "RoadNetwork.h"
#include <algorithm>

void TrafficManager::addVehicle(std::unique_ptr<Vehicule> vehicle) {
    vehicles.push_back(std::move(vehicle));
}

void TrafficManager::update(float deltaTime) {
    // If we have a network, assign leaders per segment by computing progress
    if (network) {
        // For each road segment, collect vehicles on it
        const auto& segments = network->GetRoadSegments();
        for (const auto& segPtr : segments) {
            RoadSegment* seg = segPtr.get();
            // collect (vehicle*, progress)
            std::vector<std::pair<Vehicule*, float>> onSeg;
            for (auto& vptr : vehicles) {
                Vehicule* v = vptr.get();
                float p = seg->ComputeProgressOnSegment(v->getPosition());
                if (p >= 0.0f) onSeg.emplace_back(v, p);
            }

            if (onSeg.empty()) continue;

            // sort descending so first is front-most
            std::sort(onSeg.begin(), onSeg.end(), [](const auto& a, const auto& b){ return a.second > b.second; });

            // assign leaders: vehicle i's leader is vehicle i-1 (the one in front)
            Vehicule* prev = nullptr;
            for (size_t i = 0; i < onSeg.size(); ++i) {
                Vehicule* v = onSeg[i].first;
                if (i == 0) {
                    v->setLeader(nullptr);
                } else {
                    v->setLeader(onSeg[i-1].first);
                }
            }
        }
    }

    for (auto& v : vehicles) {
        v->update(deltaTime);
    }
}

const std::vector<Vector3> TrafficManager::getVehiclePositions() const {
    std::vector<Vector3> positions;
    for (const auto& v : vehicles) {
        positions.push_back(v->getPosition());
    }
    return positions;
}

void TrafficManager::removeFinishedVehicles() {
    for (auto it = vehicles.begin(); it != vehicles.end(); ) {
        if ((*it)->hasReachedDestination()) {
            it = vehicles.erase(it);
        } else {
            ++it;
        }
    }
}

void TrafficManager::draw() {
    for (auto& v : vehicles) {
        v->draw();
    }
}
