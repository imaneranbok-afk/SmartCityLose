#include "Intersection.h"
#include "Vehicules/Vehicule.h"

Intersection::Intersection(Node* node) : node(node) {
    // Si c'est un rond-point, créer sa géométrie
    if (node->GetType() == ROUNDABOUT) {
        roundaboutGeometry = std::make_unique<RoundaboutGeometry>(
            node->GetPosition(),
            node->GetRadius(),
            node->GetRadius() * 0.4f
        );
    }
}

void Intersection::Draw() const {
    // Dessiner le rond-point si c'en est un
    if (roundaboutGeometry) {
        roundaboutGeometry->Draw();
    } else {
        // Sinon, dessiner le noeud normalement
        node->Draw();
    }
}

bool Intersection::CanEnter(Vehicule* /*vehicle*/) const {
    // TODO: Implémenter la logique de priorité
    return true;
}

void Intersection::Update(float /*deltaTime*/) {
    // TODO: Gérer les feux tricolores, priorités, etc.
}