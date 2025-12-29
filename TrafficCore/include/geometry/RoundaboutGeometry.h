#ifndef ROUNDABOUTGEOMETRY_H
#define ROUNDABOUTGEOMETRY_H

#include "RoadGeometryStrategy.h"

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

public:
    RoundaboutGeometry(Vector3 center, float radius, float roadWidth);

    void Draw() const override;
    std::vector<Vector3> GetPoints() const override;
    Vector3 GetCenter() const override { return center; }
    float GetWidth() const override { return roadWidth; }
    float GetLength() const override;

    // Pour gérer les voies sur le rond-point
    Vector3 GetLanePosition(int laneIndex, float t, int totalLanes) const;
};

#endif
