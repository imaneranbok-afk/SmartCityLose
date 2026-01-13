#ifndef ROUNDABOUTGEOMETRY_H
#define ROUNDABOUTGEOMETRY_H

#include "RoadGeometryStrategy.h"
#include "raylib.h"
#include <vector>

class RoundaboutGeometry : public RoadGeometryStrategy {
private:
    Vector3 center;
    float outerRadius;
    float innerRadius;
    float roadWidth;
    int segments;
    
    void DrawRoadCircle() const;
    void DrawCentralIsland() const;
    void DrawRoadMarkings() const;
    void DrawDirectionalArrows() const;

public:
    RoundaboutGeometry(Vector3 center, float radius, float roadWidth);
    
    void Draw() const override;
    std::vector<Vector3> GetPoints() const override;
    Vector3 GetCenter() const override { return center; }
    float GetWidth() const override { return roadWidth; }
    float GetLength() const override;
    
    float GetOuterRadius() const { return outerRadius; }
    float GetInnerRadius() const { return innerRadius; }
    
    Vector3 GetLanePosition(int laneIndex, float t, int totalLanes) const;

    // Conform to RoadGeometryStrategy interface
    void GetPositionAndTangent(float t, Vector3& pos, Vector3& tangent) const override;
};

#endif