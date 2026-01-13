# TrafficCore
une simulation de traffic capable de gérer les routes,intersections et véhicules de manière réaliste

## Chargement des modèles de véhicules
- Les classes `Vehicule`, `Car`, `Bus`, `Truck` prennent désormais en charge le chargement automatique de fichiers `.glb` depuis `assets/models/`.
- Les constructeurs acceptent des chemins contenant des espaces (ex: `"assets/models/Taxi (1).glb"`, `"assets/models/bus bleu.glb"`).
- Par défaut :
  - Bus → `assets/models/bus bleu.glb`
  - Truck → `assets/models/truck.glb`
  - Car (Taxi) → `assets/models/Taxi (1).glb`

Ces changements facilitent l'ajout de modèles sans modification du code, simplement en ajoutant les fichiers `.glb` au dossier `assets/models/`.

## Interface (API) rapide 🔧
- Vehicule constructors:
  - `Vehicule(Vector3, float maxSpd, float accel, Model, float scale, Color)` — constructeur existant.
  - `Vehicule(Vector3, float maxSpd, float accel, const std::string& modelPath, float scale, Color)` — charge le `.glb` spécifié (supporte les espaces dans le nom).
- VehiculeFactory:
  - `createVehicule(VehiculeType, Vector3)` — création rapide par type.
  - `createCar(CarModel, Vector3, int)` — création de voiture (ou bus/truck si `CarModel` l'indique). Retourne `std::unique_ptr<Vehicule>`.
  - `createFleet(...)` — crée une flotte de véhicules du même type.

> Astuce: utilisez `TrafficManager::addVehicle(std::move(v))` pour transférer la propriété d'un véhicule créé par la factory.

