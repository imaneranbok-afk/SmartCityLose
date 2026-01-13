#include "RoadSegment.h"
#include "geometry/StraightGeometry.h"
#include "geometry/CurvedGeometry.h"
#include "raymath.h"
#include "rlgl.h"
#include <cfloat>
#include <cmath>

RoadSegment::RoadSegment(Node* start, Node* end, int lanes, bool useCurvedConnection)
    : startNode(start), endNode(end), lanes(lanes), laneWidth(16.0f) {
    
    CreateGeometry(useCurvedConnection);
    CreateSidewalks();

    startNode->AddConnectedRoad(this);
    endNode->AddConnectedRoad(this);
}

void RoadSegment::CreateGeometry(bool useCurvedConnection) {
    Vector3 startPos = startNode->GetPosition();
    Vector3 endPos = endNode->GetPosition();
    float totalWidth = lanes * laneWidth;

    Vector3 direction = Vector3Normalize(Vector3Subtract(endPos, startPos));

    float startOffset = startNode->GetRadius() * 0.95f;
    float endOffset = endNode->GetRadius() * 0.95f;
    
    if (startNode->GetType() == ROUNDABOUT) {
        startOffset = fmaxf(startNode->GetRadius() * 0.8f, totalWidth * 0.5f);
    }
    if (endNode->GetType() == ROUNDABOUT) {
        endOffset = fmaxf(endNode->GetRadius() * 0.8f, totalWidth * 0.5f);
    }

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

float RoadSegment::CalculateIntersectionClearance(Node* node) const {
    float baseRadius = node->GetRadius();
    
    switch (node->GetType()) {
        case ROUNDABOUT:
            return baseRadius + 15.0f;
        
        case TRAFFIC_LIGHT:
            return baseRadius + 10.0f;
        
        case SIMPLE_INTERSECTION:
        default:
            int numConnections = node->GetConnectedRoads().size();
            if (numConnections >= 3) {
                return baseRadius + 8.0f;
            } else {
                return baseRadius + 6.0f;
            }
    }
}

void RoadSegment::CreateSidewalks() {
    auto roadPoints = geometry->GetPoints();
    if (roadPoints.size() < 2) return;

    float roadLength = GetLength();
    if (roadLength < 20.0f) return;

    float totalWidth = lanes * laneWidth;
    float sidewalkWidth = 8.0f;
    float offset = (totalWidth / 2.0f) + (sidewalkWidth / 2.0f) + 0.4f;

    float startTrimDistance = CalculateIntersectionClearance(startNode);
    float endTrimDistance = CalculateIntersectionClearance(endNode);

    if (startTrimDistance + endTrimDistance >= roadLength * 0.9f) return;

    Sidewalk leftSidewalk, rightSidewalk;
    leftSidewalk.width = sidewalkWidth;
    leftSidewalk.height = 0.15f;
    rightSidewalk.width = sidewalkWidth;
    rightSidewalk.height = 0.15f;

    float distanceAlongRoad = 0.0f;
    
    for (size_t i = 0; i < roadPoints.size(); i++) {
        if (i > 0) {
            distanceAlongRoad += Vector3Distance(roadPoints[i - 1], roadPoints[i]);
        }
        
        float distFromEnd = roadLength - distanceAlongRoad;
        
        if (distanceAlongRoad < startTrimDistance || distFromEnd < endTrimDistance) {
            continue;
        }
        
        Vector3 current = roadPoints[i];
        Vector3 direction = (i < roadPoints.size() - 1) ? 
                            Vector3Normalize(Vector3Subtract(roadPoints[i + 1], current)) : 
                            Vector3Normalize(Vector3Subtract(current, roadPoints[i - 1]));

        Vector3 right = Vector3Normalize(Vector3CrossProduct({0, 1, 0}, direction));

        leftSidewalk.path.push_back(Vector3Add(current, Vector3Scale(right, offset)));
        rightSidewalk.path.push_back(Vector3Subtract(current, Vector3Scale(right, offset)));
    }

    if (leftSidewalk.path.size() >= 2) {
        sidewalks.push_back(leftSidewalk);
    }
    if (rightSidewalk.path.size() >= 2) {
        sidewalks.push_back(rightSidewalk);
    }
}

void RoadSegment::DrawCrosswalk(Vector3 position, Vector3 direction, float roadWidth) const {
    // Paramètres du passage piéton adaptatifs
    float stripWidth = roadWidth * 0.06f;        // 6% de la largeur de route
    float stripLength = roadWidth * 0.12f;       // 12% de la largeur de route
    float stripSpacing = roadWidth * 0.08f;      // 8% de la largeur de route
    int numStrips = (int)(roadWidth / 10.0f) + 5; // Nombre adaptatif de bandes
    
    if (numStrips < 6) numStrips = 6;
    if (numStrips > 12) numStrips = 12;
    
    Vector3 right = Vector3Normalize(Vector3CrossProduct({0, 1, 0}, direction));
    
    Color stripColor = {255, 255, 255, 255};
    
    // Dessiner les bandes blanches
    for (int i = 0; i < numStrips; i++) {
        float offset = (i - numStrips/2.0f) * stripSpacing;
        Vector3 stripPos = Vector3Add(position, Vector3Scale(right, offset));
        stripPos.y += 0.03f; // Légèrement au-dessus de la route
        
        // Calculer les 4 coins de la bande
        Vector3 forward = Vector3Scale(direction, stripLength / 2.0f);
        Vector3 side = Vector3Scale(right, stripWidth / 2.0f);
        
        Vector3 p1 = Vector3Add(Vector3Add(stripPos, forward), side);
        Vector3 p2 = Vector3Add(Vector3Subtract(stripPos, forward), side);
        Vector3 p3 = Vector3Subtract(Vector3Subtract(stripPos, forward), side);
        Vector3 p4 = Vector3Subtract(Vector3Add(stripPos, forward), side);
        
        // Dessiner la bande
        rlBegin(RL_QUADS);
        rlColor4ub(stripColor.r, stripColor.g, stripColor.b, stripColor.a);
        rlVertex3f(p1.x, p1.y, p1.z);
        rlVertex3f(p2.x, p2.y, p2.z);
        rlVertex3f(p3.x, p3.y, p3.z);
        rlVertex3f(p4.x, p4.y, p4.z);
        rlEnd();
    }
}

void RoadSegment::DrawSidewalk(const Sidewalk& sidewalk) const {
    if (sidewalk.path.size() < 2) return;

    Color sidewalkColor = {180, 180, 180, 255};
    Color borderColor = {100, 100, 100, 255};

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
        rlColor4ub(sidewalkColor.r, sidewalkColor.g, sidewalkColor.b, sidewalkColor.a);
        rlVertex3f(p1.x, p1.y, p1.z);
        rlVertex3f(p2.x, p2.y, p2.z);
        rlVertex3f(p4.x, p4.y, p4.z);
        rlVertex3f(p3.x, p3.y, p3.z);
        rlEnd();

        DrawLine3D(p1, p3, borderColor);
    }
}

void RoadSegment::Draw() const {
    if (!visible) return;
    if (geometry) geometry->Draw();
    
    // Dessiner les trottoirs
    for (const auto& sidewalk : sidewalks) 
        DrawSidewalk(sidewalk);
    
    // Dessiner les passages piétons aux extrémités
    auto roadPoints = geometry->GetPoints();
    if (roadPoints.size() >= 2) {
        float roadWidth = GetWidth();
        float roadLength = GetLength();
        
        // Ne dessiner les passages piétons que sur les routes assez longues
        if (roadLength > 25.0f) {
            // Passage piéton au début
            Vector3 startPos = roadPoints.front();
            Vector3 startDir = Vector3Normalize(Vector3Subtract(roadPoints[1], roadPoints[0]));
            
            // Positionner le passage piéton légèrement en retrait
            float startOffset = startNode->GetRadius() + 3.0f;
            Vector3 crosswalkStart = Vector3Add(startPos, Vector3Scale(startDir, startOffset));
            
            DrawCrosswalk(crosswalkStart, startDir, roadWidth);
            
            // Passage piéton à la fin
            Vector3 endPos = roadPoints.back();
            Vector3 endDir = Vector3Normalize(Vector3Subtract(roadPoints[roadPoints.size()-1], 
                                                              roadPoints[roadPoints.size()-2]));
            
            float endOffset = endNode->GetRadius() + 3.0f;
            Vector3 crosswalkEnd = Vector3Subtract(endPos, Vector3Scale(endDir, endOffset));
            
            DrawCrosswalk(crosswalkEnd, endDir, roadWidth);
        }
    }
}

float RoadSegment::GetLength() const {
    return geometry ? geometry->GetLength() : 0.0f;
}

Vector3 RoadSegment::GetLanePosition(int laneIndex, float t) const {
    return GetTrafficLanePosition(laneIndex, t);
}

float RoadSegment::ComputeProgressOnSegment(const Vector3& pos) const {
    if (!geometry) return -1.0f;
    auto points = geometry->GetPoints();
    if (points.size() < 2) return -1.0f;

    // Project position onto polyline and compute distance along the polyline
    float totalLen = 0.0f;
    for (size_t i = 1; i < points.size(); ++i) totalLen += Vector3Distance(points[i-1], points[i]);
    if (totalLen <= 0.001f) return -1.0f;

    float bestDistSq = FLT_MAX;
    float distanceAlong = 0.0f; // distance from start to projection
    float accum = 0.0f;

    for (size_t i = 0; i < points.size() - 1; ++i) {
        Vector3 a = points[i];
        Vector3 b = points[i+1];
        Vector3 ab = Vector3Subtract(b, a);
        Vector3 ap = Vector3Subtract(pos, a);
        float abLenSq = Vector3LengthSqr(ab);
        float t = 0.0f;
        if (abLenSq > 0.0001f) {
            t = Vector3DotProduct(ap, ab) / abLenSq;
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;
        }
        Vector3 proj = Vector3Add(a, Vector3Scale(ab, t));
        float d2 = Vector3DistanceSqr(proj, pos);
        if (d2 < bestDistSq) {
            bestDistSq = d2;
            distanceAlong = accum + Vector3Distance(a, proj);
        }
        accum += Vector3Distance(a, b);
    }

    // If too far from the segment (e.g., off-road), return -1
    float maxAcceptDist = GetWidth() * 0.75f + 5.0f;
    if (bestDistSq > maxAcceptDist * maxAcceptDist) return -1.0f;

    return distanceAlong / totalLen;
}

// Core Direction Logic
Vector3 RoadSegment::GetDirection() const {
    Vector3 dir = Vector3Subtract(endNode->GetPosition(), startNode->GetPosition());
    return Vector3Normalize(dir);
}

// Implémentation stricte des 4 voies (2 allers, 2 retours)
Vector3 RoadSegment::GetTrafficLanePosition(int laneIndex, float t) const {
    if (geometry) {
        Vector3 pos, tangent;
        geometry->GetPositionAndTangent(t, pos, tangent);
        
        // Normale droite (Y-up) : {dir.z, 0, -dir.x}
        // Assuming tangent is normalized.
        Vector3 rightNormal = { tangent.z, 0.0f, -tangent.x };
        
        // Use the actual road visual width for positioning to center vehicles
        float effectiveWidth = this->laneWidth; 
        float offset = 0.0f;

        // Logique des voies
        if (lanes <= 2) {
            // Configuration 2 voies (1 aller, 1 retour)
            // Lane 0 doit être "Intérieure Aller" (proche du centre) pour rester sur la route (Largeur totale = 2*W)
            // Zone route : 0 à W. Centre : 0.5 * W.
            switch(laneIndex) {
                case 0: offset =  0.5f * effectiveWidth; break; // Unique voie Aller
                case 1: offset = -0.5f * effectiveWidth; break; // Unique voie Retour
                default: offset = 0.5f * effectiveWidth; break;
            }
        } else {
            // Configuration 4 voies ou plus (Standard)
            switch(laneIndex) {
                case 0: offset =  1.5f * effectiveWidth; break; // Extérieure Aller
                case 1: offset =  0.5f * effectiveWidth; break; // Intérieure Aller
                case 2: offset = -0.5f * effectiveWidth; break; // Intérieure Retour
                case 3: offset = -1.5f * effectiveWidth; break; // Extérieure Retour
                default: offset = 0.0f; break;
            }
        }

        return Vector3Add(pos, Vector3Scale(rightNormal, offset));
    }

    // Fallback if geometry is missing (legacy/straight)
    Vector3 start = startNode->GetPosition();
    Vector3 end = endNode->GetPosition();
    Vector3 dir = Vector3Normalize(Vector3Subtract(end, start));
    
    Vector3 rightNormal = { dir.z, 0.0f, -dir.x };
    
    float effectiveWidth = this->laneWidth;
    float offset = 0.0f;

    if (lanes <= 2) {
         switch(laneIndex) {
            case 0: offset =  0.5f * effectiveWidth; break;
            case 1: offset = -0.5f * effectiveWidth; break;
            default: offset = 0.5f * effectiveWidth; break;
        }
    } else {
        switch(laneIndex) {
            case 0: offset =  1.5f * effectiveWidth; break;
            case 1: offset =  0.5f * effectiveWidth; break;
            case 2: offset = -0.5f * effectiveWidth; break;
            case 3: offset = -1.5f * effectiveWidth; break;
        }
    }

    Vector3 basePos = Vector3Add(start, Vector3Scale(dir, GetLength() * t));
    return Vector3Add(basePos, Vector3Scale(rightNormal, offset));
}

