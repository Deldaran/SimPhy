# SimPhy – Simulation de galaxies sur GPU

Simulation N-corps de galaxies en OpenGL 4.3 (compute shaders). L'objectif est d'étudier
pourquoi la matière noire est nécessaire à l'existence de galaxies stables.

**Plateforme : Windows uniquement** (OpenGL 4.3 requis ; macOS s'arrête à OpenGL 4.1).

## Compilation

Prérequis : Visual Studio (ou Build Tools) avec les outils C++, CMake, et vcpkg dans `vcpkg/`
à la racine du projet (`git clone https://github.com/microsoft/vcpkg.git` puis
`.\vcpkg\bootstrap-vcpkg.bat`).

```batch
build.bat   :: installe les dépendances et compile
run.bat     :: lance build\bin\Release\GalaxyApp.exe
```

`build_windows.bat` fait la même chose avec un vcpkg installé ailleurs (`VCPKG_ROOT`, par défaut `C:\vcpkg`).

Les shaders (`shaders/`) sont lus au lancement : on peut les modifier sans recompiler.

## Structure

```
src/
├── main.cpp               Fenêtre, boucle principale, interface ImGui
├── Units.h                Système d'unités (kpc, Myr, 1e10 M☉) et constante G
├── ParticleSystem.*       Particules sur GPU (SSBO) et intégrateur leapfrog
├── InitialConditions.*    Génération du disque galactique
├── Renderer.*             Rendu des particules, bloom, tone mapping
├── Camera.*               Vue de dessus / vol libre 3D
└── Shader.*               Chargement des shaders
shaders/
├── kick.comp, drift.comp  Étapes du leapfrog
├── accel_central.comp     Gravité d'une masse centrale adoucie
└── *.vert, *.frag         Rendu
```

## Unités

| Grandeur | Unité |
|---|---|
| Longueur | 1 kpc |
| Temps | 1 Myr |
| Masse | 1e10 M☉ |
| Vitesse | 1 kpc/Myr ≈ 978 km/s |
| G | 0,044985 |

## Commandes

- Vue de dessus : glisser avec le clic gauche, molette pour zoomer, flèches ou WASD pour se déplacer
- Vol libre 3D : souris + WASD, Maj pour accélérer, Échap pour revenir
