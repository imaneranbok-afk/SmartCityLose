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

            // assign leaders within segment
            for (size_t i = 1; i < onSeg.size(); ++i) {
                onSeg[i].first->setLeader(onSeg[i-1].first);
            }

            // CROSS-SEGMENT LEADER DETECTION
            // For the front-most vehicle on this segment, find the next segment in its path 
            // and check if there's a vehicle there that should be followed.
            if (!onSeg.empty()) {
                Vehicule* frontV = onSeg[0].first;
                frontV->setLeader(nullptr); // Default for front-most

                // Check for a leader in the next segment of the itinerary
                // We'd need access to the vehicle's current segment index or just proximity.
                // Simplified Proximity Check as fallback for all vehicles to catch cross-segment issues:
                for (auto& otherPtr : vehicles) {
                    Vehicule* other = otherPtr.get();
                    if (other == frontV) continue;

                    Vector3 toOther = Vector3Subtract(other->getPosition(), frontV->getPosition());
                    float dist = Vector3Length(toOther);
                    
                    if (dist < 20.0f) { // Proximity threshold
                        Vector3 dir = {
                            sinf(frontV->getRotationAngle() * DEG2RAD),
                            0,
                            cosf(frontV->getRotationAngle() * DEG2RAD)
                        };
                        
                        float dot = Vector3DotProduct(Vector3Normalize(toOther), dir);
                        if (dot > 0.8f) { // If other is roughly in front
                             frontV->setLeader(other);
                             break;
                        }
                    }
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
