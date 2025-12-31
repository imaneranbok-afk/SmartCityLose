#include "Vehicules/ModelManager.h"

ModelManager& ModelManager::getInstance() {
    static ModelManager instance;
    return instance;
}

void ModelManager::loadModel(const std::string& category, const std::string& path) {
    // Vérifier que le fichier existe avant de tenter de le charger
    if (!FileExists(path.c_str())) {
        TraceLog(LOG_WARNING, "[ModelManager] Fichier introuvable : %s", path.c_str());
        return;
    }

    Model m = LoadModel(path.c_str());
    if (m.meshCount > 0) {
        modelLibrary[category].push_back(m);
        TraceLog(LOG_INFO, "[ModelManager] Chargé : %s -> %s", path.c_str(), category.c_str());
    } else {
        TraceLog(LOG_ERROR, "[ModelManager] Erreur chargement : %s", path.c_str());
    }
}

Model ModelManager::getRandomModel(const std::string& category) {
    if (modelLibrary.find(category) == modelLibrary.end() || modelLibrary[category].empty()) {
        return Model{0}; 
    }
    int index = GetRandomValue(0, (int)modelLibrary[category].size() - 1);
    return modelLibrary[category][index];
}

void ModelManager::unloadAll() {
    for (auto& pair : modelLibrary) {
        for (Model& m : pair.second) UnloadModel(m);
    }
    modelLibrary.clear();
}