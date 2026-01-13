#ifndef MAPLOADER_H
#define MAPLOADER_H

#include "RoadNetwork.h"
#include <string>

class MapLoader {
public:
    static bool LoadFromFile(const std::string& path, RoadNetwork& outNetwork);
};

#endif
