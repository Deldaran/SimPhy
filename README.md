# Galaxy CPU - OpenGL Project

Ce projet utilise GLFW3 et OpenGL pour créer une application graphique simple.

## Prérequis

1. **vcpkg** doit être installé dans `C:\vcpkg`
   - Cloner vcpkg : `git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg`
   - Exécuter : `C:\vcpkg\bootstrap-vcpkg.bat`
   - Intégrer avec Visual Studio : `C:\vcpkg\vcpkg integrate install`

2. **CMake** doit être installé et accessible dans le PATH

3. **Visual Studio** ou **Build Tools for Visual Studio** avec les outils C++

## Compilation

Exécutez simplement le fichier `build.bat` :

```batch
build.bat
```

Ce script va :
- Vérifier que vcpkg est installé
- Installer les dépendances (glfw3, opengl, glew)
- Configurer le projet avec CMake
- Compiler l'application

## Exécution

Une fois compilé, vous pouvez lancer l'application avec :

```batch
run.bat
```

## Structure du projet

```
Galaxy CPU/
├── src/                    # Fichiers sources (.cpp)
│   ├── main.cpp           # Point d'entrée principal
│   ├── Application.cpp    # Classe Application
│   ├── Window.cpp         # Gestion des fenêtres GLFW
│   └── Scene.cpp          # Gestion du rendu
├── include/               # Fichiers d'en-tête (.h)
│   ├── Application.h      # Interface Application
│   ├── Window.h           # Interface Window
│   └── Scene.h            # Interface Scene
├── build/                 # Dossier de compilation (généré)
├── CMakeLists.txt         # Configuration CMake
├── vcpkg.json            # Dépendances vcpkg
├── build.bat             # Script de compilation
├── run.bat               # Script d'exécution
└── README.md             # Documentation
```

## Fonctionnalités

- Fenêtre OpenGL 800x600
- Triangle coloré (rouge, vert, bleu)
- Fond bleu foncé
- Fermeture avec la touche Échap

## Dépendances

- **GLFW3** : Gestion des fenêtres et entrées
- **OpenGL** : Rendu graphique
- **GLEW** : Extensions OpenGL
