#include "MapLoader.h"
#include "../third_party/json.hpp"
#include <iostream>
#include "Vehicules/VehiculeFactory.h"

using nlohmann::json;

bool MapLoader::LoadFromFile(const std::string& path, RoadNetwork& outNetwork) {
    try {
        json doc = json::parse_file(path);
        if (doc.contains("topology") && doc["topology"].is_object()) {
            auto topo = doc["topology"];
            if (topo.contains("nodes") && topo["nodes"].is_array()) {
                for (size_t i=0;i<topo["nodes"].size();++i) {
                    auto n = topo["nodes"][i];
                    int id = n.contains("id") ? n["id"].get<int>() : (int)i+1;
                    float x = 0.0f, y=0.0f, z=0.0f;
                    if (n.contains("pos") && n["pos"].is_array()) {
                        auto arr = n["pos"];
                        x = (float)arr[static_cast<size_t>(0)].get<double>();
                        y = (float)arr[static_cast<size_t>(1)].get<double>();
                        if (arr.size() > 2) z = (float)arr[static_cast<size_t>(2)].get<double>();
                    } else if (n.contains("position") && n["position"].is_array()) {
                        auto arr = n["position"];
                        x = (float)arr[static_cast<size_t>(0)].get<double>();
                        z = (float)arr[static_cast<size_t>(1)].get<double>();
                    }
                    std::string type = "SIMPLE_INTERSECTION";
                    if (n.contains("type")) {
                        std::string t = n["type"].get<std::string>();
                        if (t == "roundabout" || t == "ROUNDABOUT") type = "ROUNDABOUT";
                        else if (t == "traffic_light" || t == "TRAFFIC_LIGHT") type = "TRAFFIC_LIGHT";
                        else type = "SIMPLE_INTERSECTION";
                    }
                    float radius = n.contains("radius") ? (float)n["radius"].get<double>() : 5.0f;
                    NodeType nt = SIMPLE_INTERSECTION;
                    if (type == "ROUNDABOUT") nt = ROUNDABOUT;
                    if (type == "TRAFFIC_LIGHT") nt = TRAFFIC_LIGHT;
                    Node* node = outNetwork.AddNode({x, 0.2f, z}, nt, radius);
                    (void)node; // id mapping not used currently
                }
            }
            if (topo.contains("routes") && topo["routes"].is_array()) {
                for (size_t i=0;i<topo["routes"].size();++i) {
                    auto e = topo["routes"][i];
                    int from = e.contains("from") ? e["from"].get<int>() : -1;
                    int to = e.contains("to") ? e["to"].get<int>() : -1;
                    int lanes = e.contains("lanes") ? e["lanes"].get<int>() : 2;
                    bool curved = e.contains("curved") ? true : false;
                    // find nodes by id (simple linear search)
                    Node* nFrom = nullptr; Node* nTo = nullptr;
                    for (auto& nptr : outNetwork.GetNodes()) {
                        if (nptr->GetId() == from) nFrom = nptr.get();
                        if (nptr->GetId() == to) nTo = nptr.get();
                    }
                        if (nFrom && nTo) {
                        RoadSegment* seg = outNetwork.AddRoadSegment(nFrom, nTo, lanes, curved);
                        if (e.contains("visible") && !e["visible"].is_null()) {
                            bool vis = true;
                            if (e["visible"].is_boolean()) vis = e["visible"].get<bool>();
                            else if (e["visible"].is_string()) vis = (e["visible"].get<std::string>() != "false");
                            
                            seg->SetVisible(vis);
                        }
                    }
                }
            }
        }
        // Read vehicle type defaults if present
        if (doc.contains("vehicle_types") && doc["vehicle_types"].is_object()) {
            auto vt = doc["vehicle_types"];
            for (const std::string& key : { std::string("CAR"), std::string("BUS"), std::string("TRUCK") }) {
                if (!vt.contains(key)) continue;
                auto node = vt[key];
                VehiculeFactory::VehicleParams p;
                if (node.contains("max_speed")) p.maxSpeed = (float)node["max_speed"].get<double>();
                if (node.contains("acceleration")) p.acceleration = (float)node["acceleration"].get<double>();
                if (node.contains("length")) p.length = (float)node["length"].get<double>();
                if (node.contains("color") && node["color"].is_string()) {
                    std::string col = node["color"].get<std::string>();
                    // parse #RRGGBB
                    if (col.size() == 7 && col[0] == '#') {
                        int r = std::stoi(col.substr(1,2), nullptr, 16);
                        int g = std::stoi(col.substr(3,2), nullptr, 16);
                        int b = std::stoi(col.substr(5,2), nullptr, 16);
                        p.color = { (unsigned char)r, (unsigned char)g, (unsigned char)b, 255 };
                    }
                }
                VehiculeType vtkey = VehiculeType::CAR;
                if (key == "BUS") vtkey = VehiculeType::BUS;
                else if (key == "TRUCK") vtkey = VehiculeType::TRUCK;
                VehiculeFactory::setDefaultParams(vtkey, p);
            }
        }
        return true;
    } catch (const std::exception& ex) {
        std::cerr << "MapLoader error: " << ex.what() << std::endl;
        return false;
    }
}
