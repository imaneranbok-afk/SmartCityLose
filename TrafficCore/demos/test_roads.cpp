#include "raylib.h"
#include "RoadNetwork.h"
#include <iostream>

int main() {
    InitWindow(800, 600, "Test Routes et Rond-Point");
    SetTargetFPS(60);

    RoadNetwork network;

    // Création nodes et rond-point via AddNode
    Node* n1 = network.AddNode({-50, 0, -50}, SIMPLE_INTERSECTION, 8.0f);
    Node* n2 = network.AddNode({50, 0, -50}, SIMPLE_INTERSECTION, 8.0f);
    Node* n3 = network.AddNode({50, 0, 50}, SIMPLE_INTERSECTION, 8.0f);
    Node* n4 = network.AddNode({-50, 0, 50}, SIMPLE_INTERSECTION, 8.0f);
    Node* nCenter = network.AddNode({0, 0, 0}, ROUNDABOUT, 10.0f);

    // Routes vers le rond-point
    network.AddRoadSegment(n1, nCenter, 2, true);
    network.AddRoadSegment(n2, nCenter, 2, true);
    network.AddRoadSegment(n3, nCenter, 2, true);
    network.AddRoadSegment(n4, nCenter, 2, true);

    // Routes périphériques
    network.AddRoadSegment(n1, n2, 3, true);
    network.AddRoadSegment(n2, n3, 3, true);
    network.AddRoadSegment(n3, n4, 3, true);
    network.AddRoadSegment(n4, n1, 3, true);

    Camera3D camera = {0};
    camera.position = {0, 80, 80};
    camera.target = {0, 0, 0};
    camera.up = {0, 1, 0};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    while (!WindowShouldClose()) {
        UpdateCamera(&camera, CAMERA_FREE);
        network.Update(GetFrameTime());

        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);
        DrawGrid(40, 10.0f);
        network.Draw();
        EndMode3D();

        DrawText("Test réseau avec lanes et rond-point", 10, 10, 20, BLACK);
        DrawFPS(700, 10);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
