#ifndef TRAFFICMANAGER_H
#define TRAFFICMANAGER_H

#include "Vehicule.h"
#include "VehiculeFactory.h"
#include <vector>
#include <memory>
#include <queue>
#include <functional>
#include <string>
#include <map>
#include "../RoadNetwork.h"

class TrafficManager {
private:
    std::vector<std::unique_ptr<Vehicule>> vehicles;
    // Optional pointer to the road network for leader assignment
    RoadNetwork* network = nullptr;

    // --- Spawner embedded ---
    struct EntryPoint {
        std::string name;
        Vector3 position;
        std::queue<VehiculeType> queue;
        EntryPoint(const std::string& n, const Vector3& p) : name(n), position(p) {}
        bool hasPending() const { return !queue.empty(); }
        void enqueue(VehiculeType t) { queue.push(t); }
        VehiculeType dequeue() { VehiculeType t = queue.front(); queue.pop(); return t; }
        int pendingCount() const { return static_cast<int>(queue.size()); }
    };

    std::vector<EntryPoint> spawnerEntries;
    size_t spawnerNextEntryIndex = 0;

    // Node-based spawn cooldown management
    std::map<int, float> nodeCooldowns; // Remaining time until next spawn allowed for node ID
    struct NodeSpawnRequest {
        int startNodeId;
        int endNodeId;
        VehiculeType type;
    };
    std::vector<NodeSpawnRequest> pendingSharedSpawns;

public:
    // Optional singleton accessor for global management (keeps existing API usable)
    static TrafficManager& getInstance();

    void addVehicle(std::unique_ptr<Vehicule> v);
    void removeFinishedVehicles();
    void update(float deltaTime);
    void draw();
    // Renvoi une référence const pour la lecture
    const std::vector<std::unique_ptr<Vehicule>>& getVehicles() const { return vehicles; }
    // Reference mutable pour modification
    std::vector<std::unique_ptr<Vehicule>>& getVehiclesCheck() { return vehicles; }
    int getVehicleCount() const { return static_cast<int>(vehicles.size()); }
    const std::vector<Vector3> getVehiclePositions() const;

    void setRoadNetwork(RoadNetwork* net) { network = net; }

    // Spawner API
    void addEntryPoint(const std::string& name, const Vector3& pos);
    void scheduleVehicles(VehiculeType type, int count);
    // Schedule an exact total of vehicles using a deterministic round‑robin across types CAR->BUS->TRUCK
    void scheduleRoundRobinVehicles(int total);
    
    // New: Factory-pattern based spawn with path
    void spawnVehicle(VehiculeType type, Vector3 startPos, std::queue<Vector3> path);

    bool spawnNext(const std::function<std::string(VehiculeType)>& modelResolver,
                   const std::function<std::vector<Vector3>(const Vector3&)>& itineraryResolver);
    void spawnAll(const std::function<std::string(VehiculeType)>& modelResolver,
                  const std::function<std::vector<Vector3>(const Vector3&)>& itineraryResolver);
    bool hasPending() const;

    // Spawn directly by node IDs (uses internal RoadNetwork pointer and PathFinder)
    bool spawnVehicleByNodeIds(int startNodeId, int endNodeId, VehiculeType type);

    // Proximity check: returns the vehicle ahead on the same segment (or nullptr). outDist filled with distance if found.
    Vehicule* checkProximity(Vehicule* v, float& outDist) const;

    // Lane offset helper: lateral offset from center for a given lane index
    static float getLaneOffset(int laneIndex, int totalLanes, float laneWidth = 3.5f);

    // Debug helper: number of queued vehicles waiting to be spawned
    int getPendingCount() const;

private:
    bool internalExecuteNodeSpawn(int startNodeId, int endNodeId, VehiculeType type);
};

#endif
