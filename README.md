# Projet OpenGL — Scène 3D Interactive

Projet réalisé en OpenGL 3.3 Core Profile (GLSL 330) par AUBRY Mathias et TOUSSAINT Nolan.

---
[image](./Computer-graphics-img.png)
## Ce qui a été implémenté

### Partie 1 — Affichage

**Chargement de modèles 3D (1.a)**  
Quatre objets sont affichés simultanément dans la scène : Kirby, Miles Morales et Gamma sont chargés depuis des fichiers `.obj` via TinyObjLoader, avec récupération automatique des matériaux depuis les fichiers `.mtl` associés (couleurs ambiante, diffuse, spéculaire, shininess) et chargement de la texture diffuse via stb_image. Le Dragon, lui, est chargé directement depuis un tableau de données brutes (fichier `.h`).

**Illumination directe — Phong (1.b)**  
Le fragment shader implémente l'équation de Phong complète : composante diffuse (Lambert) et composante spéculaire (Phong par réflexion). Une implémentation Blinn-Phong (demi-vecteur) est également présente dans le shader. Les textures et les matériaux OBJ sont combinés : si un modèle possède une texture, elle est utilisée comme couleur de base ; sinon, la couleur diffuse du matériau prend le relais.

**Illumination indirecte — Ambiante hémisphérique + Fresnel (1.c)**  
La contribution ambiante est calculée via une **lumière hémisphérique** : on interpole entre une couleur de sol et une couleur de ciel en fonction de l'orientation de la normale (`NdotSky`), ce qui donne un éclairage indirect plus naturel qu'une valeur constante. La balance entre diffuse et spéculaire est gérée par l'**approximation de Fresnel-Schlick**, avec plusieurs presets de matériau (or, eau, argent, bronze) sélectionnables à l'exécution.

**Rendu hors écran — FBO (1.d)**  
Le rendu principal s'effectue dans un Framebuffer Object (FBO) avec une texture couleur et un Renderbuffer depth/stencil. Le résultat est ensuite recopié dans le backbuffer via un quad plein écran, ce qui permet d'appliquer des post-traitements.

La correction gamma est gérée par `glEnable(GL_FRAMEBUFFER_SRGB)`.

---

### Partie 2 — Navigation

**Placement des objets (2.a)**  
Chaque modèle possède sa propre matrice monde calculée via `MakeTRSMatrix` (Translation × Rotation × Scale). Les positions, rotations et échelles sont indépendantes et ajustables en temps réel via l'interface.

**UBO Camera (2.b)**  
Les matrices de vue et de projection sont regroupées dans un Uniform Buffer Object (`CameraData`, binding point 0) partagé entre le shader principal et le shader de la skybox, évitant tout envoi redondant d'uniforms.

**Caméra orbitale**  
La caméra orbite autour de l'origine en coordonnées sphériques (rayon, theta, phi). Le clic gauche + glisser fait tourner la caméra, la molette zoom. La touche `R` réinitialise la vue.

---

### Partie 3 — Options

**Post-traitements (3.a)**  
8 effets disponibles, sélectionnables via ImGui, appliqués en plein écran sur la texture FBO :
- Aucun (passthrough)
- Inversion des couleurs
- Noir & Blanc
- Flou (box blur 3×3)
- Sépia
- Noyau de convolution (détection de contours)
- Aberration chromatique
- Pixellisation

**Skybox cubemap (3.c)**  
Une skybox est rendue via une cubemap 6 faces (PNG). Elle partage l'UBO caméra et est dessinée en dernier avec `GL_LEQUAL` pour rester en arrière-plan.

**Interface ImGui (3.e)**  
Un panneau de contrôle permet de modifier en temps réel : matériaux (ambient, diffuse, specular, shininess) et textures de chaque objet, position et échelle de chaque objet, paramètres de la caméra, effets de post-traitement, et preset de réflectivité Fresnel.

**Effet Fresnel / back-lighting (3.f & 3.g)**  
L'approximation de Schlick est implémentée dans le fragment shader pour moduler la contribution diffuse (`kD = 1 - F`) et spéculaire (`F`), assurant une conservation d'énergie approximative. Plusieurs valeurs de réflectivité F0 sont proposées en preset.

---

## Dépendances

- OpenGL 3.3, GLEW, GLFW
- [TinyObjLoader](https://github.com/tinyobjloader/tinyobjloader)
- [stb_image](https://github.com/nothings/stb)
- [Dear ImGui](https://github.com/ocornut/imgui)

---

## Contrôles

| Action | Commande |
|---|---|
| Orbiter la caméra | Clic gauche + glisser |
| Zoom | Molette souris |
| Réinitialiser la caméra | Touche `R` |
| Quitter | Touche `Échap` |
