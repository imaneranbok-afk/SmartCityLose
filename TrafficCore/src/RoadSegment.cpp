#include "RoadSegment.h"
#include "geometry/StraightGeometry.h"
#include "geometry/CurvedGeometry.h"
#include "raymath.h"
#include "rlgl.h"

RoadSegment::RoadSegment(Node* start, Node* end, int lanes, bool useCurvedConnection)
    : startNode(start), endNode(end), lanes(lanes), laneWidth(3.5f) {
    
    CreateGeometry(useCurvedConnection);
    CreateSidewalks();

    // Ajouter cette route aux noeuds connectés
    startNode->AddConnectedRoad(this);
    endNode->AddConnectedRoad(this);
}

void RoadSegment::CreateGeometry(bool useCurvedConnection) {
    Vector3 startPos = startNode->GetPosition();
    Vector3 endPos = endNode->GetPosition();
    float totalWidth = lanes * laneWidth;

    Vector3 direction = Vector3Normalize(Vector3Subtract(endPos, startPos));

    float startOffset = startNode->GetRadius();
    float endOffset = endNode->GetRadius();

    Vector3 adjustedStart = Vector3Add(startPos, Vector3Scale(direction, startOffset));
    Vector3 adjustedEnd = Vector3Add(endPos, Vector3Scale(direction, -endOffset));

    if (useCurvedConnection &&
        (startNode->GetType() == ROUNDABOUT || endNode->GetType() == ROUNDABOUT)) {
        
        Vector3 tangentStart = startNode->GetConnectionTangent(direction);
        Vector3 tangentEnd = endNode->GetConnectionTangent(Vector3Negate(direction));

        float distance = Vector3Distance(adjustedStart, adjustedEnd);
        Vector3 control1 = Vector3Add(adjustedStart, Vector3Scale(tangentStart, distance * 0.3f));
        Vector3 control2 = Vector3Add(adjustedEnd, Vector3Scale(tangentEnd, -distance * 0.3f));

        geometry = std::make_unique<CurvedGeometry>(adjustedStart, control1, control2, adjustedEnd, totalWidth);
    } else {
        geometry = std::make_unique<StraightGeometry>(adjustedStart, adjustedEnd, totalWidth, lanes);
    }
}

void RoadSegment::CreateSidewalks() {
    auto roadPoints = geometry->GetPoints();
    if (roadPoints.size() < 2) return;

    float totalWidth = lanes * laneWidth;
    float sidewalkWidth = 1.5f;
    float offset = (totalWidth / 2.0f) + (sidewalkWidth / 2.0f);

    Sidewalk leftSidewalk, rightSidewalk;
    leftSidewalk.width = sidewalkWidth;
    leftSidewalk.height = 0.2f;
    rightSidewalk.width = sidewalkWidth;
    rightSidewalk.height = 0.2f;

    for (size_t i = 0; i < roadPoints.size(); i++) {
        Vector3 current = roadPoints[i];
        Vector3 direction = (i < roadPoints.size() - 1) ? 
                            Vector3Normalize(Vector3Subtract(roadPoints[i + 1], current)) : 
                            Vector3Normalize(Vector3Subtract(current, roadPoints[i - 1]));

        Vector3 right = Vector3Normalize(Vector3CrossProduct({0, 1, 0}, direction));

        leftSidewalk.path.push_back(Vector3Add(current, Vector3Scale(right, offset)));
        rightSidewalk.path.push_back(Vector3Subtract(current, Vector3Scale(right, offset)));
    }

    sidewalks.push_back(leftSidewalk);
    sidewalks.push_back(rightSidewalk);
}

void RoadSegment::DrawSidewalk(const Sidewalk& sidewalk) const {
    if (sidewalk.path.size() < 2) return;

    for (size_t i = 0; i < sidewalk.path.size() - 1; i++) {
        Vector3 current = sidewalk.path[i];
        Vector3 next = sidewalk.path[i + 1];

        Vector3 direction = Vector3Normalize(Vector3Subtract(next, current));
        Vector3 right = Vector3Normalize(Vector3CrossProduct({0, 1, 0}, direction));

        Vector3 p1 = Vector3Add(current, Vector3Scale(right, sidewalk.width/2));
        Vector3 p2 = Vector3Subtract(current, Vector3Scale(right, sidewalk.width/2));
        Vector3 p3 = Vector3Add(next, Vector3Scale(right, sidewalk.width/2));
        Vector3 p4 = Vector3Subtract(next, Vector3Scale(right, sidewalk.width/2));

        p1.y += sidewalk.height;
        p2.y += sidewalk.height;
        p3.y += sidewalk.height;
        p4.y += sidewalk.height;

        rlBegin(RL_QUADS);
            rlColor4ub(200, 200, 200, 255);
            rlVertex3f(p1.x, p1.y, p1.z);
            rlVertex3f(p2.x, p2.y, p2.z);
            rlVertex3f(p4.x, p4.y, p4.z);
            rlVertex3f(p3.x, p3.y, p3.z);
        rlEnd();
    }
}

void RoadSegment::Draw() const {
    if (geometry) geometry->Draw();
    for (const auto& sidewalk : sidewalks) DrawSidewalk(sidewalk);
}

float RoadSegment::GetLength() const {
    return geometry ? geometry->GetLength() : 0.0f;
}

// Retourne la position sur la voie laneIndex (0 = gauche) à t = 0..1 le long de la route
Vector3 RoadSegment::GetLanePosition(int laneIndex, float t) const {
    if (!geometry || laneIndex < 0 || laneIndex >= lanes) return {0,0,0};
    
    auto points = geometry->GetPoints();
    size_t n = points.size();
    if (n < 2) return points[0];

    // Calcul de la position sur la longueur
    float total = (n-1) * t;
    size_t idx = (size_t)total;
    float localT = total - idx;

    if (idx >= n-1) idx = n-2;

    Vector3 pos = Vector3Lerp(points[idx], points[idx+1], localT);

    // Décalage selon la voie
    Vector3 dir = Vector3Normalize(Vector3Subtract(points[idx+1], points[idx]));
    Vector3 right = Vector3Normalize(Vector3CrossProduct({0,1,0}, dir));
    float laneOffset = - (GetWidth()/2.0f) + laneWidth * (0.5f + laneIndex);

    return Vector3Add(pos, Vector3Scale(right, laneOffset));
}
