#include "geometry/CurvedGeometry.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>

CurvedGeometry::CurvedGeometry(Vector3 start, Vector3 control1, Vector3 control2, Vector3 end, float width)
    : p0(start), p1(control1), p2(control2), p3(end), width(width), segments(40) {}

Vector3 CurvedGeometry::CalculateBezierPoint(float t) const {
    float u = 1 - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;
    
    Vector3 point = Vector3Scale(p0, uuu);
    point = Vector3Add(point, Vector3Scale(p1, 3 * uu * t));
    point = Vector3Add(point, Vector3Scale(p2, 3 * u * tt));
    point = Vector3Add(point, Vector3Scale(p3, ttt));
    
    return point;
}

void CurvedGeometry::Draw() const {
    DrawCurvedSurface();
    DrawCenterLine();
}

void CurvedGeometry::DrawCurvedSurface() const {
    std::vector<Vector3> curvePoints;
    for (int i = 0; i <= segments; i++) {
        float t = (float)i / segments;
        curvePoints.push_back(CalculateBezierPoint(t));
    }
    
    Color roadColor = {50, 50, 50, 255};
    
    for (size_t i = 0; i < curvePoints.size() - 1; i++) {
        Vector3 current = curvePoints[i];
        Vector3 next = curvePoints[i + 1];
        
        Vector3 direction = Vector3Subtract(next, current);
        direction = Vector3Normalize(direction);
        Vector3 right = Vector3CrossProduct({0, 1, 0}, direction);
        right = Vector3Normalize(right);
        
        Vector3 p1 = Vector3Add(current, Vector3Scale(right, width/2));
        Vector3 p2 = Vector3Subtract(current, Vector3Scale(right, width/2));
        Vector3 p3 = Vector3Add(next, Vector3Scale(right, width/2));
        Vector3 p4 = Vector3Subtract(next, Vector3Scale(right, width/2));
        
        // Surface de la route
        rlBegin(RL_QUADS);
            rlColor4ub(roadColor.r, roadColor.g, roadColor.b, roadColor.a);
            rlVertex3f(p1.x, p1.y + 0.02f, p1.z);
            rlVertex3f(p2.x, p2.y + 0.02f, p2.z);
            rlVertex3f(p4.x, p4.y + 0.02f, p4.z);
            rlVertex3f(p3.x, p3.y + 0.02f, p3.z);
        rlEnd();
    }
    
    // Lignes blanches sur les bords
    for (size_t i = 0; i < curvePoints.size() - 1; i++) {
        Vector3 current = curvePoints[i];
        Vector3 next = curvePoints[i + 1];
        
        Vector3 direction = Vector3Normalize(Vector3Subtract(next, current));
        Vector3 right = Vector3Normalize(Vector3CrossProduct({0, 1, 0}, direction));
        
        Vector3 leftCur = Vector3Add(current, Vector3Scale(right, width/2 - 0.1f));
        Vector3 leftNext = Vector3Add(next, Vector3Scale(right, width/2 - 0.1f));
        Vector3 rightCur = Vector3Subtract(current, Vector3Scale(right, width/2 - 0.1f));
        Vector3 rightNext = Vector3Subtract(next, Vector3Scale(right, width/2 - 0.1f));
        
        leftCur.y += 0.04f;
        leftNext.y += 0.04f;
        rightCur.y += 0.04f;
        rightNext.y += 0.04f;
        
        DrawLine3D(leftCur, leftNext, WHITE);
        DrawLine3D(rightCur, rightNext, WHITE);
    }
}

void CurvedGeometry::DrawCenterLine() const {
    // Ligne centrale pointillée
    std::vector<Vector3> curvePoints;
    for (int i = 0; i <= segments; i++) {
        float t = (float)i / segments;
        curvePoints.push_back(CalculateBezierPoint(t));
    }
    
    for (size_t i = 0; i < curvePoints.size() - 1; i += 3) {
        if (i + 1 < curvePoints.size()) {
            Vector3 p1 = curvePoints[i];
            Vector3 p2 = curvePoints[i + 1];
            p1.y += 0.04f;
            p2.y += 0.04f;
            DrawLine3D(p1, p2, WHITE);
        }
    }
}

std::vector<Vector3> CurvedGeometry::GetPoints() const {
    std::vector<Vector3> points;
    for (int i = 0; i <= segments; i++) {
        float t = (float)i / segments;
        points.push_back(CalculateBezierPoint(t));
    }
    return points;
}

Vector3 CurvedGeometry::GetCenter() const {
    return CalculateBezierPoint(0.5f);
}

float CurvedGeometry::GetLength() const {
    auto points = GetPoints();
    float length = 0;
    for (size_t i = 0; i < points.size() - 1; i++) {
        length += Vector3Distance(points[i], points[i + 1]);
    }
    return length;
}

void CurvedGeometry::GetPositionAndTangent(float t, Vector3& pos, Vector3& tangent) const {
    pos = CalculateBezierPoint(t);
    
    float u = 1 - t;
    float uu = u * u;
    float tt = t * t;
    
    Vector3 d0 = Vector3Subtract(p1, p0);
    Vector3 d1 = Vector3Subtract(p2, p1);
    Vector3 d2 = Vector3Subtract(p3, p2);
    
    Vector3 term1 = Vector3Scale(d0, 3 * uu);
    Vector3 term2 = Vector3Scale(d1, 6 * u * t);
    Vector3 term3 = Vector3Scale(d2, 3 * tt);
    
    Vector3 sum = Vector3Add(Vector3Add(term1, term2), term3);
    
    if (Vector3LengthSqr(sum) > 0.0001f) {
        tangent = Vector3Normalize(sum);
    } else {
        tangent = Vector3Normalize(Vector3Subtract(p3, p0));
    }
}