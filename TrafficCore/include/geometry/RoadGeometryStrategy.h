#ifndef ROADGEOMETRYSTRATEGY_H
#define ROADGEOMETRYSTRATEGY_H

#include "raylib.h"
#include <vector>

// Design Pattern: Strategy - Interface de base pour toutes les géométries
class RoadGeometryStrategy {
public:
    virtual ~RoadGeometryStrategy() = default;
    virtual void Draw() const = 0;
    virtual std::vector<Vector3> GetPoints() const = 0;
    virtual Vector3 GetCenter() const = 0;
    virtual float GetWidth() const = 0;
    virtual float GetLength() const = 0;
    virtual void GetPositionAndTangent(float t, Vector3& pos, Vector3& tangent) const = 0;
};

#endif