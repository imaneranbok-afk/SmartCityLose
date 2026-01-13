#include "geometry/StraightGeometry.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>

StraightGeometry::StraightGeometry(Vector3 start, Vector3 end, float width, int lanes)
    : start(start), end(end), width(width), lanes(lanes) {}

void StraightGeometry::Draw() const {
    DrawRoadSurface();
    DrawLaneMarkings();
    DrawEdgeLines();
}

void StraightGeometry::DrawRoadSurface() const {
    Vector3 direction = Vector3Subtract(end, start);
    direction = Vector3Normalize(direction);
    
    Vector3 right = Vector3CrossProduct({0, 1, 0}, direction);
    right = Vector3Normalize(right);
    
    Vector3 p1 = Vector3Add(start, Vector3Scale(right, width/2));
    Vector3 p2 = Vector3Subtract(start, Vector3Scale(right, width/2));
    Vector3 p3 = Vector3Add(end, Vector3Scale(right, width/2));
    Vector3 p4 = Vector3Subtract(end, Vector3Scale(right, width/2));
    
    // Surface d'asphalte
    rlBegin(RL_QUADS);
        rlColor4ub(50, 50, 50, 255);
        rlVertex3f(p1.x, p1.y + 0.02f, p1.z);
        rlVertex3f(p2.x, p2.y + 0.02f, p2.z);
        rlVertex3f(p4.x, p4.y + 0.02f, p4.z);
        rlVertex3f(p3.x, p3.y + 0.02f, p3.z);
    rlEnd();
}

void StraightGeometry::DrawEdgeLines() const {
    // Lignes blanches continues sur les bords
    Vector3 direction = Vector3Subtract(end, start);
    direction = Vector3Normalize(direction);
    
    Vector3 right = Vector3Normalize(Vector3CrossProduct({0, 1, 0}, direction));
    
    // Ligne gauche
    Vector3 leftStart = Vector3Add(start, Vector3Scale(right, width/2 - 0.1f));
    Vector3 leftEnd = Vector3Add(end, Vector3Scale(right, width/2 - 0.1f));
    leftStart.y += 0.04f;
    leftEnd.y += 0.04f;
    DrawLine3D(leftStart, leftEnd, WHITE);
    
    // Ligne droite
    Vector3 rightStart = Vector3Subtract(start, Vector3Scale(right, width/2 - 0.1f));
    Vector3 rightEnd = Vector3Subtract(end, Vector3Scale(right, width/2 - 0.1f));
    rightStart.y += 0.04f;
    rightEnd.y += 0.04f;
    DrawLine3D(rightStart, rightEnd, WHITE);
}

void StraightGeometry::DrawLaneMarkings() const {
    if (lanes <= 1) return;
    
    Vector3 direction = Vector3Subtract(end, start);
    float length = Vector3Length(direction);
    direction = Vector3Normalize(direction);
    
    Vector3 right = Vector3Normalize(Vector3CrossProduct({0, 1, 0}, direction));
    
    // Dessiner les lignes de séparation entre voies
    for (int i = 1; i < lanes; i++) {
        float offset = -width/2 + (i * width/lanes);
        Vector3 lineStart = Vector3Add(start, Vector3Scale(right, offset));
        Vector3 lineEnd = Vector3Add(end, Vector3Scale(right, offset));
        
        // Lignes pointillées blanches
        int numDashes = (int)(length / 6.0f);  // Tirets tous les 6 unités
        if (numDashes < 1) numDashes = 1;
        
        for (int j = 0; j < numDashes; j++) {
            float t1 = (float)j / numDashes;
            float t2 = t1 + 0.3f / numDashes;  // Longueur du tiret
            
            if (t2 > 1.0f) t2 = 1.0f;
            
            Vector3 dash1 = Vector3Lerp(lineStart, lineEnd, t1);
            Vector3 dash2 = Vector3Lerp(lineStart, lineEnd, t2);
            
            dash1.y += 0.04f;
            dash2.y += 0.04f;
            
            DrawLine3D(dash1, dash2, WHITE);
        }
    }
}

std::vector<Vector3> StraightGeometry::GetPoints() const {
    // Retourner plusieurs points pour une interpolation fluide
    std::vector<Vector3> points;
    int numPoints = 20;
    for (int i = 0; i <= numPoints; i++) {
        float t = (float)i / numPoints;
        points.push_back(Vector3Lerp(start, end, t));
    }
    return points;
}

Vector3 StraightGeometry::GetCenter() const {
    return Vector3Lerp(start, end, 0.5f);
}

float StraightGeometry::GetLength() const {
    return Vector3Distance(start, end);
}

void StraightGeometry::GetPositionAndTangent(float t, Vector3& pos, Vector3& tangent) const {
    pos = Vector3Lerp(start, end, t);
    tangent = Vector3Normalize(Vector3Subtract(end, start));
}