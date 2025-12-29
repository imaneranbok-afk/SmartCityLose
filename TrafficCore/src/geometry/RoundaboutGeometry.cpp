#include "RoundaboutGeometry.h"
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
}

void RoundaboutGeometry::DrawRoadCircle() const {
    rlBegin(RL_TRIANGLE_STRIP);
    rlColor4ub(50,50,50,255);
    for(int i=0;i<=segments;i++){
        float angle = (float)i/segments*2*PI;
        float cosA = cosf(angle);
        float sinA = sinf(angle);

        Vector3 outer = {center.x + outerRadius*cosA, center.y+0.01f, center.z + outerRadius*sinA};
        Vector3 inner = {center.x + innerRadius*cosA, center.y+0.01f, center.z + innerRadius*sinA};

        rlVertex3f(outer.x,outer.y,outer.z);
        rlVertex3f(inner.x,inner.y,inner.z);
    }
    rlEnd();
}

void RoundaboutGeometry::DrawCentralIsland() const {
    DrawCylinder(center, innerRadius, innerRadius, 0.3f, segments, {34,139,34,255});
}

void RoundaboutGeometry::DrawRoadMarkings() const {
    for(int i=0;i<segments;i+=4){
        float angle1 = (float)i/segments*2*PI;
        float angle2 = (float)(i+2)/segments*2*PI;

        Vector3 p1 = {center.x + outerRadius*cosf(angle1), center.y+0.02f, center.z + outerRadius*sinf(angle1)};
        Vector3 p2 = {center.x + outerRadius*cosf(angle2), center.y+0.02f, center.z + outerRadius*sinf(angle2)};
        DrawLine3D(p1,p2,WHITE);

        Vector3 p3 = {center.x + innerRadius*cosf(angle1), center.y+0.02f, center.z + innerRadius*sinf(angle1)};
        Vector3 p4 = {center.x + innerRadius*cosf(angle2), center.y+0.02f, center.z + innerRadius*sinf(angle2)};
        DrawLine3D(p3,p4,WHITE);
    }
}

std::vector<Vector3> RoundaboutGeometry::GetPoints() const {
    std::vector<Vector3> points;
    for(int i=0;i<=segments;i++){
        float angle = (float)i/segments*2*PI;
        points.push_back({center.x + outerRadius*cosf(angle), center.y, center.z + outerRadius*sinf(angle)});
    }
    return points;
}

float RoundaboutGeometry::GetLength() const {
    return 2*PI*outerRadius;
}

// Retourne la position sur une voie dans le rond-point
Vector3 RoundaboutGeometry::GetLanePosition(int laneIndex, float t, int totalLanes) const {
    float angle = t*2*PI;
    float laneOffset = -roadWidth/2.0f + roadWidth*(0.5f + laneIndex)/totalLanes;
    float r = outerRadius - laneOffset;
    return {center.x + r*cosf(angle), center.y, center.z + r*sinf(angle)};
}
