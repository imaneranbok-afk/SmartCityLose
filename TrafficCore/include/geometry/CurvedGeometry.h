#ifndef CURVEDGEOMETRY_H
#define CURVEDGEOMETRY_H

#include "RoadGeometryStrategy.h"

class CurvedGeometry : public RoadGeometryStrategy {
private:
    Vector3 p0, p1, p2, p3; // Points de contrôle de Bézier
    float width;
    int segments;
    
    Vector3 CalculateBezierPoint(float t) const;
    void DrawCurvedSurface() const;
    void DrawCenterLine() const;  // Nouvelle méthode
    
public:
    CurvedGeometry(Vector3 start, Vector3 control1, Vector3 control2, Vector3 end, float width);
    void Draw() const override;
    std::vector<Vector3> GetPoints() const override;
    Vector3 GetCenter() const override;
    float GetWidth() const override { return width; }
    float GetLength() const override;
    void GetPositionAndTangent(float t, Vector3& pos, Vector3& tangent) const override;
};

#endif