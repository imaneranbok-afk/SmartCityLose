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
        modelLibrary[category].push_back({path, m});
        TraceLog(LOG_INFO, "[ModelManager] Chargé : %s -> %s", path.c_str(), category.c_str());
    } else {
        TraceLog(LOG_ERROR, "[ModelManager] Erreur chargement : %s", path.c_str());
    }
}

Model ModelManager::getRandomModel(const std::string& category) {
    if (modelLibrary.find(category) == modelLibrary.end() || modelLibrary[category].empty()) {
        return Model{0}; 
    }
    // Deterministic: return the first model in the category for predictable behavior
    return modelLibrary[category][0].second;
}

std::vector<std::string> ModelManager::getModelPaths(const std::string& category) {
    std::vector<std::string> paths;
    if (modelLibrary.find(category) == modelLibrary.end()) return paths;
    for (auto &p : modelLibrary[category]) paths.push_back(p.first);
    return paths;
}

Model ModelManager::getModelByPath(const std::string& path) {
    for (auto &pair : modelLibrary) {
        for (auto &p : pair.second) {
            if (p.first == path) return p.second;
        }
    }
    return Model{0};
}

void ModelManager::unloadAll() {
    for (auto& pair : modelLibrary) {
        for (auto& p : pair.second) UnloadModel(p.second);
    }
    modelLibrary.clear();
}