#include "RoadNetwork.h"
#include "raylib.h"
#include <cmath>

// Fonction pour créer le réseau selon le schéma fourni
void CreateTestNetwork(RoadNetwork& network) {
    // Créer tous les noeuds avec leurs positions (X et Z échangés, puis divisés par 2)
    // Ancien: (-900, 0, 2100) -> Nouveau: (2100/2, 0, -900/2) = (1050, 0, -450)
    Node* n1 = network.AddNode({1050.0f, 0.0f, -450.0f}, SIMPLE_INTERSECTION, 20.0f);
    Node* n2 = network.AddNode({700.0f, 0.0f, -450.0f}, SIMPLE_INTERSECTION, 20.0f);
    Node* n3 = network.AddNode({700.0f, 0.0f, -250.0f}, SIMPLE_INTERSECTION, 8.0f);
    Node* n4 = network.AddNode({350.0f, 0.0f, -450.0f}, SIMPLE_INTERSECTION, 20.0f);
    Node* n5 = network.AddNode({350.0f, 0.1f, 0.0f}, ROUNDABOUT, 60.0f);
    Node* n6 = network.AddNode({0.0f, 0.1f, -450.0f}, ROUNDABOUT, 60.0f);
    Node* n7 = network.AddNode({0.0f, 0.0f, -600.0f}, SIMPLE_INTERSECTION, 20.0f);
    Node* n8 = network.AddNode({0.0f, 0.0f, 0.0f}, TRAFFIC_LIGHT, 25.0f);
    Node* n9 = network.AddNode({0.0f, 0.0f, 150.0f}, SIMPLE_INTERSECTION, 20.0f);
    Node* n10 = network.AddNode({-150.0f, 0.0f, 0.0f}, SIMPLE_INTERSECTION, 20.0f);
    
    // Ajouter les segments de route (tous droits avec false)
    network.AddRoadSegment(n1, n2, 4, false);  // Route horizontale n1->n2
    network.AddRoadSegment(n2, n3, 2, false);  // Route verticale n2->n3
    network.AddRoadSegment(n2, n4, 4, false);  // Route horizontale n2->n4
    network.AddRoadSegment(n4, n5, 4, false);  // Route verticale n4->n5
    network.AddRoadSegment(n4, n6, 4, false);  // Route horizontale n4->n6
    network.AddRoadSegment(n7, n6, 4, false);  // Route verticale n7->n6
    network.AddRoadSegment(n6, n8, 4, false);  // Route horizontale n6->n8
    network.AddRoadSegment(n8, n9, 4, false);  // Route verticale n8->n9
    network.AddRoadSegment(n8, n10, 4, false); // Route horizontale n8->n10
    network.AddRoadSegment(n5, n8, 4, false);  // Route horizontale n5->n8
    
    // Ajouter toutes les intersections
    network.AddIntersection(n1);
    network.AddIntersection(n2);
    network.AddIntersection(n3);
    network.AddIntersection(n4);
    network.AddIntersection(n5);
    network.AddIntersection(n6);
    network.AddIntersection(n7);
    network.AddIntersection(n8);
    network.AddIntersection(n9);
    network.AddIntersection(n10);
}

int main() {
    // Initialiser la fenêtre Raylib
    InitWindow(1400, 900, "Réseau Routier - 10 Intersections");
    SetTargetFPS(60);

    // Initialiser la caméra 3D (vue de dessus) - ajustée pour le nouveau réseau
    Camera3D camera;
    camera.position = {400.0f, 300.0f, -200.0f};  // Ajusté pour mieux voir le réseau
    camera.target = {400.0f, 0.0f, -200.0f};
    camera.up = {0.0f, 0.0f, -1.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Variables pour les contrôles de caméra
    float cameraSpeed = 20.0f;
    float zoomSpeed = 40.0f;

    // Créer le réseau routier
    RoadNetwork network;
    CreateTestNetwork(network);

    // Boucle principale
    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        // Contrôles de la caméra
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_Z)) {
            camera.position.z -= cameraSpeed * deltaTime;
            camera.target.z -= cameraSpeed * deltaTime;
        }
        if (IsKeyDown(KEY_S)) {
            camera.position.z += cameraSpeed * deltaTime;
            camera.target.z += cameraSpeed * deltaTime;
        }
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_Q)) {
            camera.position.x -= cameraSpeed * deltaTime;
            camera.target.x -= cameraSpeed * deltaTime;
        }
        if (IsKeyDown(KEY_D)) {
            camera.position.x += cameraSpeed * deltaTime;
            camera.target.x += cameraSpeed * deltaTime;
        }

        // Zoom
        if (IsKeyDown(KEY_UP)) {
            camera.position.y -= zoomSpeed * deltaTime;
            if (camera.position.y < 50.0f) camera.position.y = 50.0f;
        }
        if (IsKeyDown(KEY_DOWN)) {
            camera.position.y += zoomSpeed * deltaTime;
            if (camera.position.y > 600.0f) camera.position.y = 600.0f;
        }

        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            camera.position.y -= wheel * 25.0f;
            if (camera.position.y < 50.0f) camera.position.y = 50.0f;
            if (camera.position.y > 600.0f) camera.position.y = 600.0f;
        }

        // Réinitialiser
        if (IsKeyPressed(KEY_R)) {
            camera.position = {400.0f, 300.0f, -200.0f};
            camera.target = {400.0f, 0.0f, -200.0f};
        }

        // Dessin
        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);
        network.Draw();
        DrawGrid(50, 50.0f);
        EndMode3D();

        // Interface
        DrawText("Reseau Routier - 10 Intersections", 10, 10, 24, BLACK);
        DrawText("Traffic Light (n8): (0, 0, 0)", 10, 45, 16, RED);
        
        DrawText("Noeuds (X,Z):", 10, 75, 18, DARKGRAY);
        DrawText("n1: (1050, -450) - Debut", 10, 100, 13, DARKGREEN);
        DrawText("n2: (700, -450)", 10, 118, 13, DARKGREEN);
        DrawText("n3: (700, -250)", 10, 136, 13, DARKGREEN);
        DrawText("n4: (350, -450)", 10, 154, 13, DARKGREEN);
        DrawText("n5: (350, 0) - Rondpoint", 10, 172, 13, BLUE);
        DrawText("n6: (0, -450) - Rondpoint", 10, 190, 13, BLUE);
        DrawText("n7: (0, -600)", 10, 208, 13, DARKGREEN);
        DrawText("n8: (0, 0) - Traffic Light", 10, 226, 13, RED);
        DrawText("n9: (0, 150)", 10, 244, 13, DARKGREEN);
        DrawText("n10: (-150, 0)", 10, 262, 13, DARKGREEN);
        
        DrawText("10 segments de route", 10, 290, 14, DARKPURPLE);
        DrawText("Controles: ZQSD/WASD - Fleches/Molette - R", 10, 310, 14, GRAY);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}