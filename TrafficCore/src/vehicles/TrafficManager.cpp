#include "Vehicules/TrafficManager.h"
#include "Vehicules/VehiculeFactory.h"
#include "RoadNetwork.h"
#include <algorithm>
#include <iostream> // For runtime warnings when model loading fails
#include "PathFinder.h"
#include <cmath>
// For sorting utilities
#include <limits>

// --- Catmull-Rom spline smoothing helper (cubic) ---
static Vector3 catmullRomInterpolate(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;
    // standard Catmull-Rom with tension 0.5
    Vector3 out;
    out.x = 0.5f * ((2.0f * p1.x) + (-p0.x + p2.x) * t + (2.0f*p0.x - 5.0f*p1.x + 4.0f*p2.x - p3.x) * t2 + (-p0.x + 3.0f*p1.x - 3.0f*p2.x + p3.x) * t3);
    out.y = 0.5f * ((2.0f * p1.y) + (-p0.y + p2.y) * t + (2.0f*p0.y - 5.0f*p1.y + 4.0f*p2.y - p3.y) * t2 + (-p0.y + 3.0f*p1.y - 3.0f*p2.y + p3.y) * t3);
    out.z = 0.5f * ((2.0f * p1.z) + (-p0.z + p2.z) * t + (2.0f*p0.z - 5.0f*p1.z + 4.0f*p2.z - p3.z) * t2 + (-p0.z + 3.0f*p1.z - 3.0f*p2.z + p3.z) * t3);
    return out;
}

static std::vector<Vector3> CatmullRomChain(const std::vector<Vector3>& pts, int samplesPerSeg) {
    std::vector<Vector3> out;
    if (pts.size() < 2) return pts;
    // Build extended control points: duplicate endpoints
    std::vector<Vector3> p;
    p.reserve(pts.size() + 2);
    p.push_back(pts.front());
    for (auto &v : pts) p.push_back(v);
    p.push_back(pts.back());

    for (size_t i = 0; i + 3 < p.size(); ++i) {
        const Vector3 &p0 = p[i];
        const Vector3 &p1 = p[i+1];
        const Vector3 &p2 = p[i+2];
        const Vector3 &p3 = p[i+3];
        for (int s = 0; s < samplesPerSeg; ++s) {
            float t = s / (float)samplesPerSeg;
            out.push_back(catmullRomInterpolate(p0,p1,p2,p3,t));
        }
    }
    out.push_back(pts.back());
    return out;
}

// Singleton accessor implementation
TrafficManager& TrafficManager::getInstance() {
    static TrafficManager instance;
    return instance;
}

void TrafficManager::addVehicle(std::unique_ptr<Vehicule> vehicle) {
    vehicles.push_back(std::move(vehicle));
}

void TrafficManager::update(float deltaTime) {
    // === GESTION DU DÉCALAGE DE SPAWN (User Request) ===
    // 1. Mettre à jour les cooldowns des noeuds
    for (auto& pair : nodeCooldowns) {
        if (pair.second > 0) pair.second -= deltaTime;
    }

    // 2. Traiter les spawns en attente
    if (!pendingSharedSpawns.empty()) {
        auto it = pendingSharedSpawns.begin();
        while (it != pendingSharedSpawns.end()) {
            if (nodeCooldowns[it->startNodeId] <= 0) {
                if (internalExecuteNodeSpawn(it->startNodeId, it->endNodeId, it->type)) {
                    it = pendingSharedSpawns.erase(it);
                    continue; // Skip ++it
                }
            }
            ++it;
        }
    }

    // If we have a network, assign leaders per segment by computing progress
    if (network) {
        // First, clear all leaders and waiting states to start fresh each frame
        // This prevents vehicles from getting permanently stuck if they were waiting
        // for a leader or intersection that is no longer blocking them.
        for (auto& vptr : vehicles) {
            vptr->setLeader(nullptr);
            vptr->setWaiting(false);
        }
        
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

            // assign leaders within segment - UNIQUEMENT SUR LA MÊME VOIE
            for (size_t i = 0; i < onSeg.size(); ++i) {
                Vehicule* v = onSeg[i].first;
                v->setLeader(nullptr); 
                for (int j = (int)i - 1; j >= 0; --j) {
                    if (onSeg[j].first->getLane() == v->getLane()) {
                        v->setLeader(onSeg[j].first);
                        break;
                    }
                }
            }

            // --- LOGIQUE DE CHANGEMENT DE VOIE (User Request) ---
            // Si une voie est encombrée et l'autre est libre, les véhicules changent de voie.
            int forwardLanes = seg->GetLanes() / 2;
            if (forwardLanes > 1) {
                std::vector<int> laneWaitCount(forwardLanes, 0);
                for (auto& pair : onSeg) {
                    if (pair.first->isWaitingStatus()) {
                        int l = pair.first->getLane();
                        if (l < forwardLanes) laneWaitCount[l]++;
                    }
                }

                for (auto& pair : onSeg) {
                    Vehicule* v = pair.first;
                    // Uniquement si on attend, qu'on est sur une route normale et avec une probabilité de décision
                    if (v->isWaitingStatus() && v->getState() == Vehicule::State::ON_ROAD && (GetRandomValue(0, 100) < 10)) {
                        int myLane = v->getLane();
                        if (myLane >= forwardLanes) continue;

                        for (int otherLane = 0; otherLane < forwardLanes; ++otherLane) {
                            if (otherLane == myLane) continue;

                            // Si la différence est marquée (ex: 2+ véhicules)
                            if (laneWaitCount[otherLane] < laneWaitCount[myLane] - 1) {
                                // Vérification de sécurité : l'espace est-il libre dans la voie cible ?
                                bool spaceFree = true;
                                for (auto& checkPair : onSeg) {
                                    if (checkPair.first->getLane() == otherLane) {
                                        float distT = fabsf(checkPair.second - pair.second);
                                        if (distT < 0.08f) { // ~8% de la route, ajustable
                                            spaceFree = false;
                                            break;
                                        }
                                    }
                                }

                                if (spaceFree) {
                                    v->setLane(otherLane);
                                    laneWaitCount[myLane]--;
                                    laneWaitCount[otherLane]++;
                                    break; // Un changement à la fois
                                }
                            }
                        }
                    }
                }
            }

            // CROSS-SEGMENT LEADER DETECTION
            // For the front-most vehicle on this segment, find the next segment in its path 
            // and check if there's a vehicle there that should be followed.
            if (!onSeg.empty()) {
                Vehicule* frontV = onSeg[0].first;
                // frontV->setLeader(nullptr); // Already cleared above

                // Enhanced proximity check for cross-segment and waiting vehicles
                // This helps catch vehicles waiting at intersections
                Vehicule* closestAhead = nullptr;
                float closestDist = 30.0f; // Portée réduite pour éviter les interférences dans les ronds-points
                
                for (auto& otherPtr : vehicles) {
                    Vehicule* other = otherPtr.get();
                    if (other == frontV) continue;

                    Vector3 toOther = Vector3Subtract(other->getPosition(), frontV->getPosition());
                    float dist = Vector3Length(toOther);
                    
                    // IMPROVED: Use tighter distance check to prevent false leader assignments
                    // and ensure proper queue formation with correct spacing
                    if (dist < closestDist && dist > 2.0f) { // Minimum 2m to avoid self-detection issues
                        Vector3 dir = {
                            sinf(frontV->getRotationAngle() * DEG2RAD),
                            0,
                            cosf(frontV->getRotationAngle() * DEG2RAD)
                        };
                        
                        float dot = Vector3DotProduct(Vector3Normalize(toOther), dir);
                        
                        // More lenient angle for curve detection and stopped vehicles
                        // This helps detect vehicles waiting at intersections ahead
                        if (dot > 0.5f) { // Relaxed to 60 degrees for curves/roundabouts
                            closestAhead = other;
                            closestDist = dist;
                        }
                    }
                }
                
                if (closestAhead) {
                    frontV->setLeader(closestAhead);
                }
            }
        }
    }

    // Gestion des intersections (verrou simple)
    if (network) {
        const auto& intersections = network->GetIntersections();
        for (auto& vptr : vehicles) {
            Vehicule* v = vptr.get();
            bool insideAny = false;

            for (const auto& interPtr : intersections) {
                Intersection* inter = interPtr.get();
                float dist = Vector3Distance(v->getPosition(), inter->GetNode()->GetPosition());
                float radius = inter->GetNode()->GetRadius();

                // Zone d'approche ÉLARGIE pour arrêter les véhicules plus tôt et former une queue
                // Node 2 is a large intersection (radius 20), so we need a generous approach zone
                const float APPROACH_ZONE = 30.0f; // Increased from 20.0f
                
                if (dist < radius + APPROACH_ZONE && dist > radius) {
                     const auto& occ = inter->GetOccupants();
                     bool isOccupant = (std::find(occ.begin(), occ.end(), v) != occ.end());

                     if (!isOccupant) {
                         // Pour une simulation fluide, on entre directement sans attendre
                         inter->Enter(v);
                         v->setWaiting(false);
                     }
                }
                // Zone intérieure
                else if (dist <= radius) {
                    insideAny = true;
                    // S'assurer qu'on ne bloque pas (en cas de reprise)
                     const auto& occ = inter->GetOccupants();
                     bool isOccupant = (std::find(occ.begin(), occ.end(), v) != occ.end());
                     if (!isOccupant) {
                        inter->Enter(v); // Force enter si on a glitché dedans
                     }
                     v->setWaiting(false);
                }
                // Zone de sortie (juste après)
                else {
                    // Si on était occupant et qu'on est loin, on sort
                     const auto& occ = inter->GetOccupants();
                     if (std::find(occ.begin(), occ.end(), v) != occ.end()) {
                         if (dist > radius + 5.0f) {
                             inter->Exit(v);
                         }
                     }
                }
            }
        }
    }

    for (auto& v : vehicles) {
        v->update(deltaTime);
    }
    
    // Efficient cleanup using erase-remove idiom (std::erase_if equivalent for C++17)
    // Efficient cleanup using erase-remove idiom (std::erase_if equivalent for C++17)
    vehicles.erase(
        std::remove_if(vehicles.begin(), vehicles.end(),
            [this](const std::unique_ptr<Vehicule>& v) {
                if (v->readyToRemove()) {
                    // CRITICAL FIX: Remote from Intersection occupants BEFORE deletion
                    // Otherwise, the intersection thinks it's still full of dead pointers
                    // and blocks new spawns.
                    if (network) {
                        for (const auto& interPtr : network->GetIntersections()) {
                            interPtr->Exit(v.get());
                        }
                    }
                    return true; 
                }
                return false;
            }),
        vehicles.end());
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
        if ((*it)->readyToRemove()) {
            it = vehicles.erase(it);
        } else {
            ++it;
        }
    }
}

// --- Spawner implementation in TrafficManager ---
void TrafficManager::addEntryPoint(const std::string& name, const Vector3& pos) {
    spawnerEntries.emplace_back(name, pos);
}

void TrafficManager::scheduleVehicles(VehiculeType type, int count) {
    if (spawnerEntries.empty() || count <= 0) return;
    size_t idx = 0;
    for (int i = 0; i < count; ++i) {
        spawnerEntries[idx].enqueue(type);
        idx = (idx + 1) % spawnerEntries.size();
    }
    std::cout << "Scheduled " << count << " vehicles of type " << static_cast<int>(type)
              << " across " << spawnerEntries.size() << " entries (pending=" << getPendingCount() << ")" << std::endl;
}

void TrafficManager::scheduleRoundRobinVehicles(int total) {
    if (spawnerEntries.empty() || total <= 0) return;
    const std::vector<VehiculeType> types = { VehiculeType::CAR, VehiculeType::BUS, VehiculeType::TRUCK };
    size_t entryIdx = 0;
    for (int i = 0; i < total; ++i) {
        VehiculeType t = types[i % types.size()];
        spawnerEntries[entryIdx].enqueue(t);
        // If there are multiple entries, keep round-robin across entries for fairness
        entryIdx = (entryIdx + 1) % spawnerEntries.size();
    }
}

bool TrafficManager::spawnNext(const std::function<std::string(VehiculeType)>& modelResolver,
                        const std::function<std::vector<Vector3>(const Vector3&)>& itineraryResolver) {
    if (spawnerEntries.empty()) return false;

    size_t start = spawnerNextEntryIndex;
    for (size_t attempts = 0; attempts < spawnerEntries.size(); ++attempts) {
        EntryPoint& ep = spawnerEntries[spawnerNextEntryIndex];
        if (ep.hasPending()) {
            // Determine how deep this spawn is within the entry's queued vehicles
            int pendingBefore = ep.pendingCount();
            VehiculeType t = ep.dequeue();

            // Compute a small deterministic offset so multiple vehicles don't overlap visually
            Vector3 spawnPos = ep.position;
            const float spacing = 6.0f; // units between stacked spawns
            // Offset along X axis (consistent, deterministic). Deeper queued vehicles spawn further back.
            spawnPos.x += (float)(pendingBefore) * spacing;

            // Apply a small deterministic lateral offset to queued spawns so they
            // don't perfectly overlap at the entry point. Keep it minimal to avoid
            // moving vehicles off the lane — trucks get a slightly larger side offset.
            if (t == VehiculeType::TRUCK) {
                float side = ((pendingBefore % 2) == 0) ? spacing * 0.6f : -spacing * 0.6f;
                spawnPos.z += side;
            } else {
                // Small jitter for other vehicle types (keeps them near the entry lane)
                spawnPos.z += ((pendingBefore % 4) - 2) * (spacing * 0.15f);
            }

            std::string modelPath = modelResolver(t);
            std::unique_ptr<Vehicule> v;
            if (!modelPath.empty()) v = VehiculeFactory::createVehicule(t, spawnPos, modelPath);
            else v = VehiculeFactory::createVehicule(t, spawnPos);

            // Warn if a model path was provided but loading failed (meshCount == 0)
            if (!modelPath.empty() && v && !v->hasLoadedModel()) {
                std::cerr << "Warning: Failed to load model '" << modelPath << "' for vehicle type " << static_cast<int>(t) << std::endl;
            }

            if (itineraryResolver) {
                // Legacy system disabled
                // auto route = itineraryResolver(spawnPos);
                // if (!route.empty()) v->setPathFromPoints(route);
            }

            v->normalizeSize(t == VehiculeType::BUS || t == VehiculeType::TRUCK ? 16.0f : 10.0f);

            // Log spawn details
            std::cout << "Spawned vehicle type " << static_cast<int>(t)
                      << " model='" << (modelPath.empty() ? std::string("(default)") : modelPath) << "'"
                      << " loaded=" << (v->hasLoadedModel() ? "yes" : "no")
                      << " pos=(" << spawnPos.x << "," << spawnPos.y << "," << spawnPos.z << ")"
                      << std::endl;

            addVehicle(std::move(v));

            spawnerNextEntryIndex = (spawnerNextEntryIndex + 1) % spawnerEntries.size();
            return true;
        }
        spawnerNextEntryIndex = (spawnerNextEntryIndex + 1) % spawnerEntries.size();
    }

    spawnerNextEntryIndex = start;
    return false;
}

void TrafficManager::spawnAll(const std::function<std::string(VehiculeType)>& modelResolver,
                       const std::function<std::vector<Vector3>(const Vector3&)>& itineraryResolver) {
    while (hasPending()) {
        bool ok = spawnNext(modelResolver, itineraryResolver);
        (void)ok;
    }
}

void TrafficManager::spawnVehicle(VehiculeType type, Vector3 startPos, std::queue<Vector3> path) {
    // 1. Create vehicle using the Factory (returns std::unique_ptr<Vehicule>)
    auto newVehicle = VehiculeFactory::createVehicule(type, startPos);
    
    // 2. Assign path if valid
    if (newVehicle && !path.empty()) {
        // Legacy path assignment disabled
        /*
        // Convert queue to vector for setPathFromPoints
        std::vector<Vector3> pathVec;
        while (!path.empty()) {
            pathVec.push_back(path.front());
            path.pop();
        }
        newVehicle->setPathFromPoints(pathVec);
        */
        
        // Ensure correct size/params
        if (VehiculeFactory::hasDefaultParams(type)) {
             auto p = VehiculeFactory::getDefaultParams(type);
             newVehicle->setMaxSpeed(p.maxSpeed);
        }
    }

    // 3. Add to the managed container
    if (newVehicle) {
        addVehicle(std::move(newVehicle));
    }
}

bool TrafficManager::hasPending() const {
    for (const auto& e : spawnerEntries) if (e.hasPending()) return true;
    return false;
}

int TrafficManager::getPendingCount() const {
    int c = 0;
    for (const auto& e : spawnerEntries) c += static_cast<int>(e.queue.size());
    return c;
}

void TrafficManager::draw() {
    for (auto& v : vehicles) {
        v->draw();
    }
}

bool TrafficManager::spawnVehicleByNodeIds(int startNodeId, int endNodeId, VehiculeType type) {
    // Si un cooldown est actif pour ce noeud, on met en attente
    if (nodeCooldowns.count(startNodeId) > 0 && nodeCooldowns[startNodeId] > 0.0f) {
        pendingSharedSpawns.push_back({startNodeId, endNodeId, type});
        return true; // Donnée acceptée pour le futur
    }

    // Sinon on tente le spawn immédiat
    return internalExecuteNodeSpawn(startNodeId, endNodeId, type);
}

bool TrafficManager::internalExecuteNodeSpawn(int startNodeId, int endNodeId, VehiculeType type) {
    auto& nodes = network->GetNodes();
    Node* startNode = nullptr;
    Node* endNode = nullptr;

    for (auto& n : nodes) {
        if (n->GetId() == startNodeId) startNode = n.get();
        if (n->GetId() == endNodeId) endNode = n.get();
    }

    if (!startNode || !endNode) return false;

    // --- ENFORCE STRICT SPAWN/DESPAWN NODES ---
    auto isAllowedNode = [](const Vector3& p) -> bool {
        const std::vector<Vector3> allowed = {
            {1050.0f, 0.0f, -450.0f},
            {700.0f, 0.0f, -250.0f},
            {0.0f, 0.0f, -600.0f},
            {0.0f, 0.0f, 150.0f},
            {-150.0f, 0.0f, 0.0f}
        };
        for (const auto& a : allowed) {
            if (fabs(p.x - a.x) < 5.0f && fabs(p.z - a.z) < 5.0f) return true;
        }
        return false;
    };

    if (!isAllowedNode(startNode->GetPosition()) || !isAllowedNode(endNode->GetPosition())) {
        return false;
    }

    PathFinder pf(network);
    std::vector<Node*> nodePath = pf.FindPath(startNode, endNode);
    if (nodePath.empty()) return false;

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
        if (connecting) roadRoute.push_back(connecting);
    }
    if (roadRoute.empty()) return false;

    RoadSegment* firstRoad = roadRoute.front();
    int forwardLanes = firstRoad->GetLanes() / 2; 
    if (forwardLanes < 1) forwardLanes = 1;
    int chosenLane = GetRandomValue(0, forwardLanes - 1); 
    Vector3 spawnPos = roadRoute.front()->GetTrafficLanePosition(chosenLane, 0.0f);
    
    // Check if spawn position is clear of other vehicles to avoid instant collision
    for (auto& vptr : vehicles) {
        if (Vector3Distance(vptr->getPosition(), spawnPos) < 25.0f) {
            // Still too close to another vehicle, postpone
            return false;
        }
    }

    auto veh = VehiculeFactory::createVehicule(type, spawnPos);
    if (!veh) return false;

    veh->setLane(chosenLane);
    veh->setRoute(roadRoute);
    veh->normalizeSize(type == VehiculeType::BUS || type == VehiculeType::TRUCK ? 16.0f : 10.0f);
    addVehicle(std::move(veh));

    // Activation du cooldown pour ce noeud (ex: 2.5 secondes entre chaque spawn unique sur ce noeud)
    nodeCooldowns[startNodeId] = 2.5f;

    std::cout << "[SPAWN] Node " << startNodeId << " -> " << endNodeId << " (OK, Cooldown activated)" << std::endl;
    return true;
}

Vehicule* TrafficManager::checkProximity(Vehicule* v, float& outDist) const {
    outDist = std::numeric_limits<float>::infinity();
    if (!network || !v) return nullptr;

    // Find segment containing v (use ComputeProgressOnSegment)
    const auto& segments = network->GetRoadSegments();
    for (const auto& segPtr : segments) {
        RoadSegment* seg = segPtr.get();
        float p = seg->ComputeProgressOnSegment(v->getPosition());
        if (p < 0.0f) continue;

        // collect other vehicles on same segment
        Vehicule* best = nullptr;
        float bestDelta = std::numeric_limits<float>::infinity();
        for (const auto& otherPtr : vehicles) {
            Vehicule* o = otherPtr.get();
            if (o == v) continue;
            float op = seg->ComputeProgressOnSegment(o->getPosition());
            if (op < 0.0f) continue;
            float delta = op - p;
            if (delta > 0 && delta < bestDelta) {
                bestDelta = delta;
                best = o;
            }
        }
        if (best) {
            // compute world distance
            outDist = Vector3Distance(best->getPosition(), v->getPosition());
            return best;
        }
        // if none ahead on this segment, no proximity to report
        return nullptr;
    }
    return nullptr;
}

float TrafficManager::getLaneOffset(int laneIndex, int totalLanes, float laneWidth) {
    float center = -((float)totalLanes * laneWidth) / 2.0f;
    return center + laneWidth * (0.5f + laneIndex);
}

// checkPriority method removed for fluidity
