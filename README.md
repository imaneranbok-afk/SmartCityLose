# 🌆 Smart City Traffic Simulation

Une simulation de trafic urbain intelligente et fluide développée en **C++** avec la bibliothèque **Raylib**. Ce projet met l'accent sur le réalisme des trajectoires, la gestion avancée des ronds-points et un comportement de conduite fluide (Smart Driving).

![Raylib](https://img.shields.io/badge/Graphics-Raylib-blue)
![C++](https://img.shields.io/badge/Language-C++17-orange)
![License](https://img.shields.io/badge/License-MIT-green)

---

## ✨ Fonctionnalités Clés

### 🚗 Physique et Comportement des Véhicules
- **Trajectoires Fluides** : Utilisation de courbes de Bézier quadratiques pour les virages aux intersections et d'interpolations en spirale pour les entrées de rond-point.
- **Zéro Saccade** : Machine à états optimisée pour garantir une continuité de mouvement sans micro-arrêts lors des transitions (Same-Frame State Processing).
- **Centrage de Voie** : Les véhicules s'alignent mathématiquement au milieu des couloirs de circulation (0.90x et 0.70x du rayon pour les ronds-points).
- **Gestion des Distances** : Système de car-following réaliste avec freinage progressif et arrêt de sécurité.

### 🔄 Infrastructures Intelligentes
- **Ronds-points Avancés** : Gestion déterministe de la distance angulaire restante pour garantir des sorties précises à chaque trajet.
- **Priorités de Circulation** : Logique de "Yield" (Céder le passage) intégrée détectant les véhicules prioritaires dans l'anneau.
- **Navigation par Graphes** : Système de Pathfinder permettant aux véhicules de trouver le chemin le plus court entre deux points de la ville.

### 🎨 Graphismes et Rendu
- **Rendu 3D Premium** : Modèles de véhicules texturés avec gestion des offsets de pivot pour des virages réalistes.
- **Géométrie Dynamique** : Génération de marquages routiers, flèches directionnelles et îlots centraux texturés.

---

## 🛠 Technologies Utilisées

- **C++17** : Pour une gestion mémoire efficace et des performances optimales.
- **Raylib** : Pour le moteur de rendu 3D et la gestion des entrées.
- **CMake** : Système de build multi-plateforme.
- **Ninja/MinGW** : Pour la compilation rapide.

---

## 🏗 Architecture du Projet

Le projet est structuré de manière modulaire :
- `TrafficCore/src/vehicles` : Logique de physique, de décision et gestionnaire de trafic (`TrafficManager`).
- `TrafficCore/src/geometry` : Rendu des structures routières complexes (ronds-points).
- `TrafficCore/include` : Définitions des classes et interfaces.
- `RoadNetwork` : Gestion de la topologie de la ville et du graphe de navigation.

---

## 🚀 Installation et Utilisation

### Prérequis
- Un compilateur C++ supportant le C++17 (MinGW-w64, MSVC, Clang).
- [CMake](https://cmake.org/download/) (version 3.10 ou supérieure).
- [Raylib](https://www.raylib.com/) (généralement inclus ou géré par CMake).

### Compilation
1. Clonez le repository :
   ```bash
   git clone https://github.com/votre-username/smart-city-simulation.git
   cd smart-city-simulation
   ```
2. Créez un dossier de build et compilez :
   ```bash
   mkdir build
   cd build
   cmake .. -G "Ninja" # ou "MinGW Makefiles"
   cmake --build .
   ```
3. Lancez l'application :
   ```bash
   ./SmartCity.exe
   ```

---

## 🎮 Contrôles et Commandes

Pour interagir avec la simulation, utilisez les touches suivantes :

### 🚗 Gestion du Trafic
- **V** : **Ajouter un véhicule** – Spawne instantanément un nouveau véhicule (Voiture, Bus ou Camion) à l'un des points d'entrée (N1, N3, N7, N9, N10).
- **K** : **Supprimer un véhicule** – Supprime le véhicule actuellement suivi par la caméra ou, à défaut, le dernier véhicule ajouté.
- **SPACE** : **Pause / Reprise** – Fige ou relance le mouvement de tous les véhicules.

### 🎥 Caméra et Navigation
- **Touches 1, 2, 3** : Changer le mode de vue :
  - `1` : **Mode Orbital (RTS)** – Rotation autour d'un point focal.
  - `2` : **Mode Vol Libre (FPS)** – Déplacement libre avec la souris et le clavier.
  - `3` : **Suivi de Véhicule** – La caméra s'attache à une voiture spécifique.
- **Z / Q / S / D** (ou **W / A / S / D**) : Déplacer la caméra ou faire un "Pan" au sol.
- **Souris (Clic Droit)** : Rotation orbitale.
- **Molette Souris / Touches E-Q** : Zoom avant / arrière.
- **TAB** : Changer de véhicule (uniquement en mode suivi `3`).
- **C** : Activer le **Mode Cinématique** (rotation automatique lente).
- **R** : **Reset** – Réinitialise la caméra à sa position par défaut.

### ⚙️ Menu
- **M** : Retourner au **Menu de Configuration** pour redéfinir le nombre de véhicules et les modèles 3D avant de relancer.

---

## 📄 Licence

Distribué sous la licence MIT. Voir `LICENSE` pour plus d'informations.

---

## 👥 Membres du Groupe

Ce projet a été réalisé en collaboration par :

- **Ouarda Ait El Fakih** - [@ouarda-aitelafkih](https://github.com/ouarda-aitelafkih)
- **Fatima Zahra El Bouhssini** - [@fatimazahraeelbouhssini4-arch](https://github.com/fatimazahraeelbouhssini4-arch)
- **Halima Achabbak** - [@achabbakhalima-ai](https://github.com/achabbakhalima-ai)
- **Fatima Boumehaout** - [@Fatimaboumehaout](https://github.com/Fatimaboumehaout)

---
*Projet réalisé dans le cadre de l'Atelier 2 de Programmation Orientée Objet.*
