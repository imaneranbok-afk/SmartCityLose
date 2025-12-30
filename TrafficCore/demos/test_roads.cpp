#include "RoadNetwork.h"
#include "raylib.h"
#include "raymath.h"
#include <cmath>

// Fonction pour créer le réseau selon le schéma fourni
void CreateTestNetwork(RoadNetwork& network) {
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
    
    network.AddRoadSegment(n1, n2, 4, false);
    network.AddRoadSegment(n2, n3, 2, false);
    network.AddRoadSegment(n2, n4, 4, false);
    network.AddRoadSegment(n4, n5, 4, false);
    network.AddRoadSegment(n4, n6, 4, false);
    network.AddRoadSegment(n7, n6, 4, false);
    network.AddRoadSegment(n6, n8, 4, false);
    network.AddRoadSegment(n8, n9, 4, false);
    network.AddRoadSegment(n8, n10, 4, false);
    network.AddRoadSegment(n5, n8, 4, false);
    
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
    InitWindow(1400, 900, "Réseau Routier - Caméra Libre 3D");
    SetTargetFPS(60);

    // ========== CAMÉRA LIBRE TYPE JEU VIDEO ==========
    Camera3D camera = {0};
    camera.position = {0.0f, 50.0f, 0.0f};     // Position de départ (0, 50, 0)
    camera.target = {0.0f, 0.0f, 10.0f};       // Regarde vers l'avant
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    Model fst = LoadModel("../assets/models/school_building.glb");
    Vector3 fstPos = { 10.0f, 0.01f, -5.0f };
    float fstScale = 0.1f;

    // Variables de contrôle de caméra
    float yaw = 0.0f;           // Rotation horizontale (gauche/droite)
    float pitch = -20.0f;       // Rotation verticale (haut/bas)
    float moveSpeed = 30.0f;    // Vitesse de déplacement
    float sprintSpeed = 60.0f;  // Vitesse en sprint (Shift)
    float mouseSensitivity = 0.15f;
    
    bool freeCameraMode = true;  // Mode caméra libre activé
    
    DisableCursor();  // Masquer et capturer le curseur

    // Créer le réseau routier
    RoadNetwork network;
    CreateTestNetwork(network);

    // Boucle principale
    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        // ========== MODE CAMÉRA LIBRE ==========
        if (freeCameraMode) {
            // Contrôle de la souris (rotation)
            Vector2 mouseDelta = GetMouseDelta();
            yaw -= mouseDelta.x * mouseSensitivity;
            pitch -= mouseDelta.y * mouseSensitivity;
            
            // Limiter le pitch (ne pas retourner la caméra)
            if (pitch > 89.0f) pitch = 89.0f;
            if (pitch < -89.0f) pitch = -89.0f;

            // Calculer la direction de la caméra
            Vector3 forward = {
                cosf(pitch * DEG2RAD) * sinf(yaw * DEG2RAD),
                sinf(pitch * DEG2RAD),
                cosf(pitch * DEG2RAD) * cosf(yaw * DEG2RAD)
            };
            forward = Vector3Normalize(forward);

            Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));

            // Vitesse de déplacement (Sprint avec Shift)
            float currentSpeed = IsKeyDown(KEY_LEFT_SHIFT) ? sprintSpeed : moveSpeed;

            // Déplacement ZQSD/WASD
            if (IsKeyDown(KEY_W) || IsKeyDown(KEY_Z)) {
                camera.position = Vector3Add(camera.position, Vector3Scale(forward, currentSpeed * deltaTime));
            }
            if (IsKeyDown(KEY_S)) {
                camera.position = Vector3Subtract(camera.position, Vector3Scale(forward, currentSpeed * deltaTime));
            }
            if (IsKeyDown(KEY_A) || IsKeyDown(KEY_Q)) {
                camera.position = Vector3Subtract(camera.position, Vector3Scale(right, currentSpeed * deltaTime));
            }
            if (IsKeyDown(KEY_D)) {
                camera.position = Vector3Add(camera.position, Vector3Scale(right, currentSpeed * deltaTime));
            }

            // Monter/Descendre (Espace/Ctrl)
            if (IsKeyDown(KEY_SPACE)) {
                camera.position.y += currentSpeed * deltaTime;
            }
            if (IsKeyDown(KEY_LEFT_CONTROL)) {
                camera.position.y -= currentSpeed * deltaTime;
                if (camera.position.y < 1.0f) camera.position.y = 1.0f;  // Ne pas descendre sous le sol
            }

            // Mettre à jour la cible de la caméra
            camera.target = Vector3Add(camera.position, forward);
        }

        // ========== RACCOURCIS CLAVIER ==========
        
        // TAB : Activer/Désactiver le mode caméra libre
        if (IsKeyPressed(KEY_TAB)) {
            freeCameraMode = !freeCameraMode;
            if (freeCameraMode) {
                DisableCursor();
            } else {
                EnableCursor();
            }
        }

        // R : Réinitialiser la caméra à la position de départ
        if (IsKeyPressed(KEY_R)) {
            camera.position = {0.0f, 50.0f, 0.0f};
            camera.target = {0.0f, 0.0f, 10.0f};
            yaw = 0.0f;
            pitch = -20.0f;
        }

        // 1 : Vue de dessus (top-down)
        if (IsKeyPressed(KEY_ONE)) {
            camera.position = {400.0f, 400.0f, -200.0f};
            camera.target = {400.0f, 0.0f, -200.0f};
            camera.up = {0.0f, 0.0f, -1.0f};
            yaw = 0.0f;
            pitch = -89.0f;
        }

        // 2 : Vue Traffic Light (n8)
        if (IsKeyPressed(KEY_TWO)) {
            camera.position = {50.0f, 30.0f, 50.0f};
            camera.target = {0.0f, 0.0f, 0.0f};
            camera.up = {0.0f, 1.0f, 0.0f};
        }

        // 3 : Vue panoramique
        if (IsKeyPressed(KEY_THREE)) {
            camera.position = {500.0f, 200.0f, 500.0f};
            camera.target = {0.0f, 0.0f, 0.0f};
            camera.up = {0.0f, 1.0f, 0.0f};
        }

        // ESC : Quitter (alternative)
        if (IsKeyPressed(KEY_ESCAPE)) {
            break;
        }
        Color skyColor = {135, 206, 235, 255};     // Bleu ciel clair
        Color groundColor = {100, 150, 100, 255};  // Vert herbe clair
        // ========== RENDU ==========
        BeginDrawing(); 
        ClearBackground(skyColor); // Ciel bleu

        BeginMode3D(camera);
        
        // Dessiner le réseau routier
        network.Draw();
        
        // Grille de référence
        //DrawGrid(100, 50.0f);
        
        // Axes de référence (optionnel)
        DrawLine3D({0, 0, 0}, {50, 0, 0}, RED);    // Axe X
        DrawLine3D({0, 0, 0}, {0, 50, 0}, GREEN);  // Axe Y
        DrawLine3D({0, 0, 0}, {0, 0, 50}, BLUE);   // Axe Z
        DrawPlane((Vector3){0.0f, -0.5f, 0.0f}, (Vector2){5000.0f, 5000.0f}, groundColor);
        DrawModel(fst, fstPos, fstScale, WHITE);

        EndMode3D();

        // ========== INTERFACE ==========
        DrawRectangle(0, 0, 350, 420, {0, 0, 0, 150});
        
        DrawText("=== RESEAU ROUTIER 3D ===", 10, 10, 20, WHITE);
        
        DrawText("CAMERA:", 10, 45, 16, YELLOW);
        DrawText(TextFormat("Position: (%.1f, %.1f, %.1f)", 
                 camera.position.x, camera.position.y, camera.position.z), 10, 65, 14, WHITE);
        DrawText(TextFormat("Yaw: %.1f | Pitch: %.1f", yaw, pitch), 10, 85, 14, WHITE);
        
        DrawText("CONTROLES:", 10, 115, 16, YELLOW);
        DrawText("ZQSD/WASD : Deplacer", 10, 135, 13, WHITE);
        DrawText("Souris    : Regarder", 10, 152, 13, WHITE);
        DrawText("Espace    : Monter", 10, 169, 13, WHITE);
        DrawText("Ctrl      : Descendre", 10, 186, 13, WHITE);
        DrawText("Shift     : Sprint (2x vitesse)", 10, 203, 13, WHITE);
        DrawText("TAB       : Mode camera libre ON/OFF", 10, 220, 13, LIME);
        
        DrawText("RACCOURCIS:", 10, 250, 16, YELLOW);
        DrawText("R : Reinitialiser camera", 10, 270, 13, WHITE);
        DrawText("1 : Vue de dessus", 10, 287, 13, WHITE);
        DrawText("2 : Vue Traffic Light", 10, 304, 13, WHITE);
        DrawText("3 : Vue panoramique", 10, 321, 13, WHITE);
        DrawText("ESC : Quitter", 10, 338, 13, WHITE);
        
        DrawText("RESEAU:", 10, 368, 16, YELLOW);
        DrawText("10 Intersections | 2 Ronds-points", 10, 388, 13, WHITE);
        DrawText("1 Traffic Light  | 10 Segments", 10, 405, 13, WHITE);
        

        // Indicateur du mode caméra
        if (freeCameraMode) {
            DrawText("[ CAMERA LIBRE ACTIVE ]", 365, 10, 16, LIME);
            DrawText("+ CROIX AU CENTRE +", 650, 445, 12, {255, 255, 255, 150});
            DrawLine(700, 450, 700, 450, RED);  // Réticule
            DrawCircle(700, 450, 2, RED);
        } else {
            DrawText("[ CAMERA FIXE ]", 365, 10, 16, ORANGE);
        }

        DrawText(TextFormat("FPS: %d", GetFPS()), 1320, 870, 16, LIME);


        EndDrawing();
    }

    EnableCursor();
    CloseWindow();
    return 0;
}