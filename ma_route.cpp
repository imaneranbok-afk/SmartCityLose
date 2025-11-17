#include <raylib.h>
#include <raymath.h>
#include<rlgl.h>

// Couleurs personnalisées
Color COULEUR_CIEL = {135, 206, 235, 255};
Color COULEUR_HERBE = {34, 139, 34, 255};
Color COULEUR_ASPHALTE = {50, 50, 50, 255};
Color COULEUR_TROTTOIRS = {224, 212, 193, 255};
Color COULEUR_VOITURE =RED;

// Fonction pour dessiner la route
void DrawRoute(){
    float largeur_route = 8.0f;
    float longueur_route = 50.0f;

    // 1. Route principale (asphalte)
    DrawCube((Vector3){0, 0.01f, 0}, largeur_route, 0.02f, longueur_route, COULEUR_ASPHALTE);

    // 2. Trottoirs
    float largeur_trottoir = 1.5f;
    float hauteur_trottoir = 0.2f;
    
    // Trottoir de droite
    float position_trottoir_droite = -largeur_route/2 - largeur_trottoir/2;
    DrawCube((Vector3){position_trottoir_droite, hauteur_trottoir/2 + 0.01f, 0}, 
             largeur_trottoir, hauteur_trottoir, longueur_route, COULEUR_TROTTOIRS);
    
    // Trottoir de gauche
    float position_trottoir_gauche = largeur_route/2 + largeur_trottoir/2;
    DrawCube((Vector3){position_trottoir_gauche, hauteur_trottoir/2 + 0.01f, 0}, 
             largeur_trottoir, hauteur_trottoir, longueur_route, COULEUR_TROTTOIRS);

    // 3. Lignes blanches sur les bords
    float largeur_ligne = 0.2f;
    float hauteur_ligne = 0.03f;
    
    // Ligne blanche de gauche
    float position_gauche = -largeur_route/2 + largeur_ligne/2;
    DrawCube((Vector3){position_gauche, 0.02f, 0}, largeur_ligne, hauteur_ligne, longueur_route, WHITE);
    
    // Ligne blanche de droite
    float position_droite = largeur_route/2 - largeur_ligne/2;
    DrawCube((Vector3){position_droite, 0.02f, 0}, largeur_ligne, hauteur_ligne, longueur_route, WHITE);
    
    // 4. Ligne jaune centrale (pointillée)
    float longueur_trait = 3.0f;
    float longueur_espace = 3.0f;
    float longueur_segment = longueur_trait + longueur_espace;
    int nombre_segments = (int)(longueur_route / longueur_segment);
    
    for (int i = -nombre_segments/2; i < nombre_segments/2; i++) {
        float positionZ = i * longueur_segment;
        // Vérifier que le trait est dans les limites de la route
        if (positionZ >= -longueur_route/2 && positionZ <= longueur_route/2) {
            DrawCube((Vector3){0, 0.02f, positionZ}, largeur_ligne, hauteur_ligne, longueur_trait, YELLOW);
        }
    }
}
void DrawRoue(Vector3 position,float rayon,Color coleur,int Segments){
    rlPushMatrix();
    rlTranslatef(position.x,position.y,position.z);
    rlRotatef(90.0f,0.0f,0.0f,1.0f);
    float epaisseur=rayon * 1.5;
    DrawCylinder((Vector3){0,0,0},rayon,rayon,epaisseur,Segments,coleur);
    rlPopMatrix();
}

int main(){
    const int largeur_fenetre = 1400;
    const int hauteur_fenetre = 800;
    InitWindow(largeur_fenetre, hauteur_fenetre, "ma_route_3D");

    Camera3D camera = {0};
    camera.position = (Vector3){0.0f, 10.0f, 10.0f};
    camera.target = (Vector3){0.0f, 0.0f, 0.0f};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    SetTargetFPS(60);
    
    while(!WindowShouldClose()){
        // Contrôles caméra
        if (IsKeyDown(KEY_RIGHT)) camera.position.x += 0.1f;
        if (IsKeyDown(KEY_LEFT)) camera.position.x -= 0.1f;
        if (IsKeyDown(KEY_UP)) camera.position.z -= 0.1f;
        if (IsKeyDown(KEY_DOWN)) camera.position.z += 0.1f;
        if (IsKeyDown(KEY_W)) camera.position.y += 0.1f;
        if (IsKeyDown(KEY_S)) camera.position.y -= 0.1f;
        
        BeginDrawing();
            ClearBackground(COULEUR_CIEL);
            BeginMode3D(camera);
                DrawPlane((Vector3){0, 0, 0}, (Vector2){50, 50}, COULEUR_HERBE);
                DrawRoute();
                DrawGrid(30, 2.0f);
                DrawRoue((Vector3){0.2,0.7,0},0.7f,BLACK,25);
                DrawRoue((Vector3){0.576,0.7,0},0.7f,BLACK,25);
                DrawRoue((Vector3){0.2,0.7,0.96},0.7f,BLACK,25);
                DrawRoue((Vector3){0.576,0.7,0.96},0.7f,BLACK,25);
            EndMode3D();
            
            DrawText("Ma ROUTE 3D - Fleches pour bouger, Z/S pour monter/descendre", 10, 10, 20, BLACK);
            DrawFPS(largeur_fenetre - 100, 10);
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}