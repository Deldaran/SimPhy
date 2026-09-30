# SimPhy – Feuille de route

Objectif : une simulation N-corps de galaxies, sur GPU, qui permet de **comprendre par l'expérience**
pourquoi la matière noire est nécessaire, et quel est le vrai rôle de l'énergie noire.
Des effets de relativité générale simplifiés sont ajoutés là où ils changent réellement le résultat.

Règle de travail : pour chaque étape, **expliquer la méthode avant de coder**, puis **valider**
le résultat par un test dont on connaît la réponse.

---

## Rappels de physique (à garder en tête)

1. **À l'échelle d'une galaxie, Newton suffit.**
   - v ≈ 200 km/s, donc v/c ≈ 10⁻³ : les corrections relativistes sont de l'ordre de 10⁻⁶.
   - La relativité générale (RG) intervient seulement :
     - près des trous noirs (phase 6) ;
     - en cosmologie, via l'expansion de l'Univers et les équations de Friedmann (phase 5).
2. **La matière noire et l'énergie noire jouent des rôles opposés.**
   - Sans matière noire :
     - les courbes de rotation observées sont impossibles ;
     - un disque seul est instable (il forme une barre, instabilité d'Ostriker–Peebles, 1973) ;
     - les structures n'ont pas le temps de se former depuis le Big Bang.
   - L'énergie noire **freine** la formation des structures.
     - Sans elle, il se formerait *plus* de galaxies, pas moins.
     - Elle explique l'accélération de l'expansion et l'âge de l'Univers.
3. **Les étoiles ne se percutent pas : une galaxie est un système non collisionnel.**
   - Temps de relaxation : t_relax ≈ N / (8 ln N) × t_traversée, soit des milliers de fois l'âge de l'Univers.
   - Conséquences pour la simulation :
     - **pas de friction** entre étoiles ;
     - un **adoucissement ε** de la gravité, parce que chaque particule représente ~10⁶ étoiles.
   - La friction ne reviendra que pour une éventuelle composante « gaz », traitée plus tard.

---

## État actuel : phase 0 terminée ✅

- [x] Windows uniquement, OpenGL 4.3 core, compute shaders + SSBO (le code Mac a été supprimé)
- [x] Code découpé en modules (`src/`) et shaders dans des fichiers séparés (`shaders/`, lus au lancement)
- [x] Unités : kpc, Myr, 1e10 M☉, donc **G = 0,044985** (`src/Units.h`), et 1 kpc/Myr ≈ 978 km/s
- [x] Intégrateur **leapfrog KDK** (kick → drift → accélérations → kick), symplectique
- [x] **Pas de temps fixe** (résultats reproductibles, indépendants des FPS)
- [x] Générateur aléatoire `std::mt19937` avec graine réglable
- [x] Disque exponentiel Σ ∝ e^(−R/R_d), rayon tiré par une loi Gamma(2, R_d)
- [x] Seule force pour l'instant : une masse centrale avec adoucissement de Plummer (`shaders/accel_central.comp`)
- [x] Indicateur ω·dt dans l'interface (stable si < 2, précis si < 0,1)
- [x] Contexte OpenGL de débogage : les erreurs du pilote s'affichent dans la console

### ⚠️ À faire en premier sur Windows

Le code compile et les shaders passent `glslangValidator`, mais **il n'a jamais été exécuté**
(le Mac ne dispose que d'OpenGL 4.1).

1. `build.bat` puis `run.bat`.
2. Vérifier que la console affiche `OpenGL 4.x` et le nom du bon GPU (le GPU dédié sur un portable).
3. Vérifier qu'aucun message `[GL ERREUR]` n'apparaît et qu'aucun shader n'est en erreur.
4. **Test de référence :**
   - Réglages : masse centrale = 10, σ_v = 0.
   - Résultat attendu : le disque tourne sans se déformer, et les étoiles de chaque rayon restent sur leur cercle.
   - Avec σ_v = 30 km/s, les motifs doivent s'étirer en spirale : c'est la rotation différentielle, un effet normal.
5. Tester le redimensionnement de la fenêtre, le vol libre 3D (Échap pour sortir) et le bouton de réinitialisation.

---

## Phase 1 : vraie gravité newtonienne (auto-gravité)

### 1a. Sommation directe sur GPU (référence exacte)

**Méthode :**
- Chaque particule additionne la force de toutes les autres :
  a_i = Σ_j G m_j (r_j − r_i) / (|r_j − r_i|² + ε²)^(3/2)
- Coût en O(N²) : environ 16 000 à 65 000 particules en temps réel.
- **Tuilage en mémoire partagée** (*shared memory tiling*, GPU Gems 3, chap. 31) :
  1. Chaque groupe de 256 threads charge un bloc de 256 particules en mémoire `shared`, qui est rapide.
  2. Chaque thread calcule ses 256 interactions avec ce bloc.
  3. On passe au bloc suivant.
- Cette organisation divise les accès à la mémoire lente par 256.

**Tâches :**
- [ ] `shaders/accel_direct.comp`, avec `shared vec4 tile[256]` et `barrier()`
- [ ] `ParticleSystem::ComputeAccelerations` : accélération totale = masse centrale (si > 0) + auto-gravité
- [ ] Choix de la méthode de gravité dans l'interface
- [ ] Utiliser `pos.w`, la masse de chaque particule, qui existe déjà

**Validation :**
- [ ] Deux corps en orbite circulaire : l'orbite reste fermée.
- [ ] Sphère de Plummer à l'équilibre (profil analytique connu) : elle ne s'effondre pas et ne s'évapore pas.

### 1b. Particle-Mesh (PM) pour 10⁶ particules

**Méthode, en trois étapes :**
1. **Dépôt de masse CIC (Cloud-In-Cell)** sur une grille 3D de 128³ :
   - chaque particule répartit sa masse sur les 8 cellules voisines, avec des poids linéaires ;
   - l'écriture concurrente se fait avec `atomicAdd` sur des entiers en virgule fixe, ou `GL_NV_shader_atomic_float` si disponible.
2. **Équation de Poisson ∇²Φ = 4πGρ résolue par FFT :**
   - dans l'espace de Fourier, elle devient Φ̂(k) = −4πG ρ̂(k) / k² ;
   - galaxie isolée : grille doublée à 256³, remplie de zéros autour (méthode de Hockney–Eastwood), pour éviter les copies périodiques de la galaxie ;
   - la FFT est écrite en compute shader (radix-2, trois passes 1D selon x, y et z), ou calculée sur CPU avec `pocketfft` pour commencer.
3. **Force et interpolation :**
   - a = −∇Φ par différences finies centrées ;
   - la force est ramenée aux particules avec le **même noyau CIC** que le dépôt (sinon, une particule s'attire elle-même).

**Validation :**
- [ ] Comparer PM et sommation directe sur le même nuage de particules. Écart attendu : moins de quelques % au-delà de 2 cellules.

---

## Phase 2 : diagnostics (le leapfrog est déjà en place)

**Méthode :** des réductions parallèles en compute shader (sommes sur toutes les particules), puis une lecture
de quelques valeurs seulement côté CPU. Les graphes sont tracés dans ImGui (`PlotLines`).

- [ ] **Énergie totale** E = K + W :
  - K = Σ ½ m v² ;
  - W = ½ Σ m Φ, ou Σ m Φ_ext pour un potentiel externe.
  - C'est **le test de validité du code** : E doit rester constante à mieux que 1 %.
- [ ] **Moment cinétique** L = Σ m r × v : doit être conservé.
- [ ] **Rapport du viriel** 2K/|W| : vaut 1 à l'équilibre. Au-dessus de 1, le système n'est pas lié et se disperse.
- [ ] **Courbe de rotation mesurée** v_c(R), par anneaux de rayon :
  - tracée à côté de la courbe prédite par la seule matière visible ;
  - c'est le graphique clé de tout le projet.
- [ ] Profil de densité Σ(R) et épaisseur du disque au cours du temps (mesure du « chauffage » du disque).

---

## Phase 3 : galaxies en équilibre (disque + bulbe + halo)

**Composantes :**
- [ ] **Disque exponentiel** (existe déjà ; il faut le rendre auto-gravitant).
- [ ] **Bulbe de Hernquist** : ρ(r) = M a / (2π r (r + a)³).
- [ ] **Halo de matière noire NFW** : ρ(r) = ρ₀ / [(r/r_s)(1 + r/r_s)²]. Activable, masse réglable.
  - D'abord comme **potentiel analytique fixe** (un shader `accel_halo.comp`) ;
  - puis comme **halo vivant** fait de particules, nécessaire pour que le halo réagisse au disque (échange de moment cinétique avec la barre).

**Conditions initiales à l'équilibre :**
- [ ] **Vitesse circulaire** v_c²(R) = R · dΦ_total/dR, avec la contribution de *toutes* les composantes.
- [ ] **Dispersion des vitesses** par les **équations de Jeans**, l'équivalent de l'équilibre hydrostatique pour un « gaz » d'étoiles :
  - σ_R, σ_φ via l'approximation épicyclique ;
  - σ_z depuis l'épaisseur du disque ;
  - vitesse de rotation moyenne corrigée de la « dérive asymétrique » (*asymmetric drift*).
- [ ] **Paramètre de Toomre** Q = σ_R κ / (3,36 G Σ) :
  - κ est la fréquence épicyclique ;
  - on règle Q ≈ 1,2 à 1,5 (comme la Voie lactée) ; en dessous de 1, le disque se fragmente.
- [ ] Préréglage « Voie lactée » :
  - disque de 5, bulbe de 1, halo de ~100 (en unités de 1e10 M☉) ;
  - R_d = 3 kpc, r_s ≈ 20 kpc.

---

## Phase 4 : expériences sur la matière noire (le cœur du projet)

| Exp. | Réglage | Résultat attendu | Référence historique |
|---|---|---|---|
| **A** | Matière visible seule, mais vitesses initiales plates à 220 km/s (ce qu'on observe) | 2K/\|W\| > 1 : les étoiles du bord s'échappent, la galaxie se disperse | Zwicky (1933), Rubin (1970) |
| **B** | Matière visible seule, vitesses cohérentes avec elle | Une **barre** se forme en quelques rotations, puis le disque chauffe et s'épaissit | Ostriker & Peebles (1973) |
| **C** | Avec halo NFW (~10 fois la masse du disque) | Courbe de rotation plate, disque stable, bras spiraux transitoires | Modèle standard ΛCDM |
| **D** | MOND (gravité modifiée à faible accélération, sans matière noire) | Courbes plates également | Contre-hypothèse à tester honnêtement |

**Mesures :**
- [ ] **Force de la barre** : A₂ = |Σ e^(2iθ)| / N, calculé dans le disque interne et tracé en fonction du temps.
- [ ] Courbe de rotation mesurée contre courbe prédite (phase 2).
- [ ] Bouton « scénario » qui charge chaque expérience d'un clic, avec une graine fixe pour pouvoir comparer.

**Pour l'expérience D (MOND) :** montrer à la fois ce qu'elle explique (les courbes de rotation) et ce qu'elle
n'explique pas (la dynamique des amas de galaxies, la formation des structures en phase 5).

---

## Phase 5 : cosmologie, formation des structures (RG via Friedmann)

**Méthodes :**
- [ ] **Boîte périodique en coordonnées comobiles**, qui suivent l'expansion. La FFT de la phase 1b est naturellement périodique, donc elle s'applique sans zéros autour.
- [ ] **Équation de Friedmann**, déduite de la RG :
  - H(a)² = H₀² (Ω_m a⁻³ + Ω_Λ) ;
  - on intègre a(t), le facteur d'échelle.
- [ ] **Équation du mouvement en comobile** :
  - dp/dt = −∇φ / a, avec p = a² dx/dt ;
  - l'expansion agit comme un frein, le « frottement de Hubble ».
- [ ] **Conditions initiales par l'approximation de Zel'dovich** :
  - on génère un champ gaussien de fluctuations à partir d'un spectre de puissance simplifié (loi de puissance ou approximation BBKS) ;
  - puis on déplace les particules d'une grille régulière selon ce champ.

**Expériences :**

| Scénario | Paramètres | Résultat attendu |
|---|---|---|
| Baryons seuls | Ω_m = 0,05, fluctuations δ ~ 10⁻⁵ au découplage | δ ∝ a, donc ×1100 seulement : δ ~ 0,01 aujourd'hui. **Aucune structure.** |
| ΛCDM | Ω_m = 0,3, Ω_Λ = 0,7 | La matière noire a commencé à s'effondrer avant le découplage : on obtient la **toile cosmique**, avec filaments et halos. |
| Matière noire sans Λ | Ω_m = 1 | *Plus* de structures, et un Univers trop jeune. Cela montre le vrai rôle de l'énergie noire. |

---

## Phase 6 : relativité générale « simplifiée » près des trous noirs

- [ ] **Potentiel de Paczyński–Wiita** Φ = −GM / (r − r_s) :
  - reproduit la dernière orbite circulaire stable (ISCO) à 3 r_s ;
  - en dessous, la matière plonge dans le trou noir.
- [ ] **Correction post-newtonienne 1PN** : précession du périhélie.
  - Test : une orbite de type Mercure doit donner ~43″ par siècle, avec les bonnes unités.
- [ ] **Lentilles gravitationnelles**, dans le rendu :
  - déviation des rayons de lumière α = 4GM / (c² b) ;
  - anneaux d'Einstein ;
  - le halo de matière noire devient « visible » par la distorsion du fond.

---

## Améliorations techniques en attente

- [ ] Bloom : ajouter un filtre de luminosité (seuil) avant le flou, et flouter à demi-résolution.
- [ ] Sauvegarde et chargement d'un état de simulation (fichier binaire des particules + paramètres).
- [ ] Capture d'écran et de vidéo (séquence d'images) pour documenter les expériences.
- [ ] `imgui.ini` contient encore les fenêtres de l'ancien projet ; on peut le supprimer.
- [ ] `build.bat` et `build_windows.bat` sont presque identiques ; les fusionner.

## Ordre conseillé

1. **Phase 0** : vérifier le lancement sous Windows.
2. **1a** : sommation directe.
3. **2** : diagnostics, surtout l'énergie et la courbe de rotation.
4. **3** : galaxies en équilibre.
5. **4** : expériences A, B, C (la démonstration principale).
6. **1b** : Particle-Mesh, pour passer à 10⁶ particules.
7. **5** : cosmologie.
8. **6** : relativité près des trous noirs.
