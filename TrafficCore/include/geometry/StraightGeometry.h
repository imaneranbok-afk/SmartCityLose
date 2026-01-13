#ifndef STRAIGHTGEOMETRY_H
#define STRAIGHTGEOMETRY_H

#include "RoadGeometryStrategy.h"

class StraightGeometry : public RoadGeometryStrategy {
private:
    Vector3 start;
    Vector3 end;
    float width;
    int lanes;
    
    void DrawRoadSurface() const;
    void DrawLaneMarkings() const;
    void DrawEdgeLines() const;  // Nouvelle méthode
    
public:
    StraightGeometry(Vector3 start, Vector3 end, float width, int lanes);
    void Draw() const override;
    std::vector<Vector3> GetPoints() const override;
    Vector3 GetCenter() const override;
    float GetWidth() const override { return width; }
    float GetLength() const override;
    void GetPositionAndTangent(float t, Vector3& pos, Vector3& tangent) const override;
};

#endif