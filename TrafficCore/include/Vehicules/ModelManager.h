#ifndef MODEL_MANAGER_H
#define MODEL_MANAGER_H

#include "raylib.h"
#include <map>
#include <vector>
#include <string>

class ModelManager {
private:
    std::map<std::string, std::vector<Model>> modelLibrary;
    ModelManager() {} // Constructeur privé (Singleton)

public:
    static ModelManager& getInstance();
    
    // Charge un modèle dans une catégorie (ex: "CAR", "assets/models/car1.glb")
    void loadModel(const std::string& category, const std::string& path);
    
    // Récupère un modèle aléatoire parmi ceux chargés dans une catégorie
    Model getRandomModel(const std::string& category);
    
    // Libère la mémoire GPU
    void unloadAll();

    // Suppression de la copie pour le Singleton
    ModelManager(const ModelManager&) = delete;
    void operator=(const ModelManager&) = delete;
};

#endif