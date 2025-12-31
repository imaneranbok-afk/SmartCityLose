#include "geometry/RoundaboutGeometry.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

RoundaboutGeometry::RoundaboutGeometry(Vector3 center, float radius, float roadWidth)
    : center(center), roadWidth(roadWidth), segments(64) {
    this->outerRadius = radius;
    this->innerRadius = radius - roadWidth;
}

void RoundaboutGeometry::Draw() const {
    DrawRoadCircle();
    DrawCentralIsland();
    DrawRoadMarkings();
    DrawDirectionalArrows();
}

void RoundaboutGeometry::DrawRoadCircle() const {
    Color roadColor = {50, 50, 50, 255};
    
    // Dessiner le cercle de route avec transitions douces
    rlBegin(RL_TRIANGLES);
    rlColor4ub(roadColor.r, roadColor.g, roadColor.b, roadColor.a);
    
    for(int i = 0; i < segments; i++) {
        float angle1 = (float)i / segments * 2 * PI;
        float angle2 = (float)(i + 1) / segments * 2 * PI;
        
        float cos1 = cosf(angle1);
        float sin1 = sinf(angle1);
        float cos2 = cosf(angle2);
        float sin2 = sinf(angle2);
        
        Vector3 outer1 = {center.x + outerRadius * cos1, center.y + 0.01f, center.z + outerRadius * sin1};
        Vector3 inner1 = {center.x + innerRadius * cos1, center.y + 0.01f, center.z + innerRadius * sin1};
        Vector3 outer2 = {center.x + outerRadius * cos2, center.y + 0.01f, center.z + outerRadius * sin2};
        Vector3 inner2 = {center.x + innerRadius * cos2, center.y + 0.01f, center.z + innerRadius * sin2};
        
        rlVertex3f(outer1.x, outer1.y, outer1.z);
        rlVertex3f(inner1.x, inner1.y, inner1.z);
        rlVertex3f(outer2.x, outer2.y, outer2.z);
        
        rlVertex3f(inner1.x, inner1.y, inner1.z);
        rlVertex3f(inner2.x, inner2.y, inner2.z);
        rlVertex3f(outer2.x, outer2.y, outer2.z);
    }
    rlEnd();
    
    // Dessiner des lignes blanches sur les bords extérieurs pour délimiter
    rlBegin(RL_LINES);  // CHANGÉ ICI : RL_LINE_STRIP -> RL_LINES
    rlColor4ub(255, 255, 255, 180);
    for(int i = 0; i < segments; i++) {  // CHANGÉ: <= en 
        float angle1 = (float)i / segments * 2 * PI;
        float angle2 = (float)(i + 1) / segments * 2 * PI;
        
        Vector3 p1 = {
            center.x + outerRadius * cosf(angle1),
            center.y + 0.03f,
            center.z + outerRadius * sinf(angle1)
        };
        Vector3 p2 = {
            center.x + outerRadius * cosf(angle2),
            center.y + 0.03f,
            center.z + outerRadius * sinf(angle2)
        };
        rlVertex3f(p1.x, p1.y, p1.z);
        rlVertex3f(p2.x, p2.y, p2.z);
    }
    rlEnd();
}

void RoundaboutGeometry::DrawCentralIsland() const {
    Color islandColor = {34, 139, 34, 255};
    Color borderColor = {255, 255, 255, 255};
    
    // Îlot central
    DrawCylinder(
        {center.x, center.y + 0.2f, center.z},
        innerRadius * 0.95f,
        innerRadius * 0.95f,
        0.4f,
        segments,
        islandColor
    );
    
    // Bordure blanche autour de l'îlot
    rlBegin(RL_LINES);  // CHANGÉ ICI : RL_LINE_STRIP -> RL_LINES
    rlColor4ub(borderColor.r, borderColor.g, borderColor.b, borderColor.a);
    for(int i = 0; i < segments; i++) {  // CHANGÉ: <= en 
        float angle1 = (float)i / segments * 2 * PI;
        float angle2 = (float)(i + 1) / segments * 2 * PI;
        
        Vector3 p1 = {
            center.x + innerRadius * cosf(angle1),
            center.y + 0.03f,
            center.z + innerRadius * sinf(angle1)
        };
        Vector3 p2 = {
            center.x + innerRadius * cosf(angle2),
            center.y + 0.03f,
            center.z + innerRadius * sinf(angle2)
        };
        rlVertex3f(p1.x, p1.y, p1.z);
        rlVertex3f(p2.x, p2.y, p2.z);
    }
    rlEnd();
}

void RoundaboutGeometry::DrawRoadMarkings() const {
    float midRadius = (outerRadius + innerRadius) / 2.0f;
    
    // Lignes pointillées au centre de la voie
    for(int i = 0; i < segments; i += 3) {
        float angle1 = (float)i / segments * 2 * PI;
        float angle2 = (float)(i + 1.5f) / segments * 2 * PI;
        
        Vector3 p1 = {
            center.x + midRadius * cosf(angle1),
            center.y + 0.02f,
            center.z + midRadius * sinf(angle1)
        };
        Vector3 p2 = {
            center.x + midRadius * cosf(angle2),
            center.y + 0.02f,
            center.z + midRadius * sinf(angle2)
        };
        
        DrawLine3D(p1, p2, WHITE);
    }
}

void RoundaboutGeometry::DrawDirectionalArrows() const {
    int numArrows = 6;
    float arrowRadius = (outerRadius + innerRadius) / 2.0f;
    Color arrowColor = {255, 255, 255, 230};
    
    for (int i = 0; i < numArrows; i++) {
        float angle = (float)i / numArrows * 2 * PI;
        
        Vector3 arrowPos = {
            center.x + arrowRadius * cosf(angle),
            center.y + 0.03f,
            center.z + arrowRadius * sinf(angle)
        };
        
        float tangentAngle = angle + PI/2;
        Vector3 direction = {cosf(tangentAngle), 0, sinf(tangentAngle)};
        
        float arrowLength = 2.0f;
        float arrowWidth = 0.4f;
        
        Vector3 tip = {
            arrowPos.x + direction.x * arrowLength,
            arrowPos.y,
            arrowPos.z + direction.z * arrowLength
        };
        
        Vector3 perpendicular = {-direction.z, 0, direction.x};
        
        Vector3 left = {
            tip.x - direction.x * 0.7f + perpendicular.x * arrowWidth,
            tip.y,
            tip.z - direction.z * 0.7f + perpendicular.z * arrowWidth
        };
        
        Vector3 right = {
            tip.x - direction.x * 0.7f - perpendicular.x * arrowWidth,
            tip.y,
            tip.z - direction.z * 0.7f - perpendicular.z * arrowWidth
        };
        
        rlBegin(RL_TRIANGLES);
        rlColor4ub(arrowColor.r, arrowColor.g, arrowColor.b, arrowColor.a);
        rlVertex3f(tip.x, tip.y, tip.z);
        rlVertex3f(left.x, left.y, left.z);
        rlVertex3f(right.x, right.y, right.z);
        rlEnd();
        
        Vector3 bodyStart = {
            arrowPos.x - direction.x * 0.5f,
            arrowPos.y,
            arrowPos.z - direction.z * 0.5f
        };
        
        float bodyWidth = 0.2f;
        Vector3 b1 = Vector3Add(bodyStart, Vector3Scale(perpendicular, bodyWidth));
        Vector3 b2 = Vector3Subtract(bodyStart, Vector3Scale(perpendicular, bodyWidth));
        Vector3 b3 = Vector3Add(left, Vector3Scale(direction, 0.3f));
        Vector3 b4 = Vector3Add(right, Vector3Scale(direction, 0.3f));
        
        rlBegin(RL_QUADS);
        rlColor4ub(arrowColor.r, arrowColor.g, arrowColor.b, arrowColor.a);
        rlVertex3f(b1.x, b1.y, b1.z);
        rlVertex3f(b2.x, b2.y, b2.z);
        rlVertex3f(b4.x, b4.y, b4.z);
        rlVertex3f(b3.x, b3.y, b3.z);
        rlEnd();
    }
}

std::vector<Vector3> RoundaboutGeometry::GetPoints() const {
    std::vector<Vector3> points;
    for(int i = 0; i <= segments; i++) {
        float angle = (float)i / segments * 2 * PI;
        points.push_back({
            center.x + outerRadius * cosf(angle),
            center.y,
            center.z + outerRadius * sinf(angle)
        });
    }
    return points;
}

float RoundaboutGeometry::GetLength() const {
    return 2 * PI * outerRadius;
}

Vector3 RoundaboutGeometry::GetLanePosition(int laneIndex, float t, int totalLanes) const {
    float angle = t * 2 * PI;
    float laneOffset = -roadWidth / 2.0f + roadWidth * (0.5f + laneIndex) / totalLanes;
    float r = outerRadius - laneOffset;
    return {
        center.x + r * cosf(angle),
        center.y,
        center.z + r * sinf(angle)
    };
}