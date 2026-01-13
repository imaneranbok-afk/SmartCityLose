#ifndef MODEL_MANAGER_H
#define MODEL_MANAGER_H

#include "raylib.h"
#include <map>
#include <vector>
#include <string>

class ModelManager {
private:
    std::map<std::string, std::vector<std::pair<std::string, Model>>> modelLibrary;
    ModelManager() {} // Constructeur privé (Singleton)

public:
    static ModelManager& getInstance();
    
    // Charge un modèle dans une catégorie (ex: "CAR", "assets/models/car1.glb").
    // Le chemin peut contenir des espaces (ex: "assets/models/Taxi (1).glb").
    // Exemple: loadModel("CAR", "assets/models/Taxi (1).glb");
    void loadModel(const std::string& category, const std::string& path);
    
    // Récupère un modèle aléatoire parmi ceux chargés dans une catégorie
    Model getRandomModel(const std::string& category);

    // Récupère la liste des chemins de modèles chargés dans la catégorie
    std::vector<std::string> getModelPaths(const std::string& category);

    // Récupère un modèle par chemin (utile si vous voulez charger un modèle spécifique)
    Model getModelByPath(const std::string& path);
    
    // Libère la mémoire GPU
    void unloadAll();

    // Suppression de la copie pour le Singleton
    ModelManager(const ModelManager&) = delete;
    void operator=(const ModelManager&) = delete;
};

#endif