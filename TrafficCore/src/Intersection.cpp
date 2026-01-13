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

bool Intersection::CanEnter(Vehicule* /*v*/) const {
    // Pour une simulation fluide, on autorise l'entrée multiple.
    // La régulation se fait par la distance de sécurité individuelle des véhicules.
    return true;
}

void Intersection::Enter(Vehicule* v) {
    if (v && std::find(occupants.begin(), occupants.end(), v) == occupants.end()) {
        occupants.push_back(v);
    }
}

void Intersection::Exit(Vehicule* v) {
    auto it = std::find(occupants.begin(), occupants.end(), v);
    if (it != occupants.end()) {
        occupants.erase(it);
    }
}

void Intersection::Update(float /*deltaTime*/) {
    // Nettoyage de sécurité : si un véhicule est détruit ou invalide
    // (Dans une vraie simu, faudrait vérifier si v est toujours vivant)
}