# Images et animations — version 0.2

## Ce qui est intégré

Les **15 images sources retenues** sont dans `ArtSource/` : sept planches du héros (marche, repos, combat corrigé, sort, assis, blessé, chute), trois planches idle des monstres et cinq chara designs (héros, frère en tenue élégante, Mousse, Roncinet, Luciolle). Les anciennes variantes remplacées ne sont pas utilisées.

| Image | Utilisation dans le projet |
|---|---|
| Héros idle / marche | Sélection automatique selon le déplacement, quatre directions relatives à la caméra |
| Héros combat | Geste déclenché par l'action Attaquer |
| Héros spell | Geste magique déclenché par Tendre la main ; cela n'ajoute pas un nouveau système de sorts |
| Héros assis / blessé / chute | Aperçus de trois secondes accessibles dans Carnet → page du héros |
| Mousse idle | Apparence du compagnon de départ |
| Roncinet idle | Monstre visible dans la zone et recrutable |
| Luciolle idle | Monstre visible près des roseaux, recrutable pour la quête |
| Cinq chara designs | Pages consultables dans le Carnet en jeu |

Les sept animations du héros peuvent être prévisualisées depuis le Carnet. Le frère a une planche consultable mais pas encore de sprites ni de personnage jouable. Les monstres n'ont que leur animation de repos : le compagnon reste animé en idle lorsqu'il accompagne le héros. Les aperçus blessé/chute ne créent pas de système de PV ou de mort pour le héros.

## Premier lancement Unreal

1. Mettre le dépôt à jour et fermer toute ancienne instance de l'éditeur.
2. Ouvrir `FinalTower.uproject` avec UE **5.8**, puis compiler les modules Editor si demandé.
3. Le module **FinalTowerEditor** lit `ArtSource/manifest.json` après l'initialisation du moteur. Il importe les PNG, crée **160 PaperSprites, 40 PaperFlipbooks et 15 textures** dans `/Game/GeneratedArt`, puis sauvegarde les packages. Il n'est pas nécessaire de découper les planches à la main ou d'installer Python pour cette étape.
4. Attendre la fin de l'import avant Play. Le journal doit indiquer `FINAL TOWER art ready`. Les lancements suivants réutilisent les packages existants si le manifeste et les fichiers attendus n'ont pas changé.
5. Lancer Play : le héros et les monstres doivent apparaître avec leurs sprites. Une forme grise à leur place signale un import absent/échoué ; consulter `LogFinalTowerArt` et `LogFinalTowerSprites`.

L'importeur écrit uniquement dans `Content/GeneratedArt/` et `Saved/FinalTowerArt/`. Le dossier GeneratedArt est reproductible et ignoré par Git, pour éviter de versionner des centaines de packages que le moteur peut reconstruire. Les PNG sources et les atlas préparés sont versionnés directement ; ils ne nécessitent pas Git LFS. Les autres futurs assets Unreal peuvent utiliser LFS conformément au `.gitattributes`.

## Cuisson mobile

`/Game/GeneratedArt` figure dans `DirectoriesToAlwaysCook`, car les animations et pages sont chargées par leur chemin dans le code. **Ouvrir le projet et terminer l'import une première fois avant le premier packaging**, puis vérifier que les packages existent. Le module d'import est Editor uniquement : le jeu Android/iOS charge des assets cuits normaux et n'exécute ni Python, ni détection d'images, ni import de PNG au lancement.

## Préparation reproductible des planches

Les feuilles générées ne forment pas toutes une grille exacte : certaines font 1254 px et des silhouettes dépassent les divisions mathématiques d'une grille 4×4. `Tools/prepare_art.py` détecte les **16 silhouettes réelles** par composantes connexes alpha, les classe en quatre rangées, rattache les petits fragments proches et élimine uniquement les pixels presque transparents de l'extérieur. Cela évite de couper des têtes, des oreilles ou des ailes.

Les originaux restent intacts. Les images préparées sont des atlas **1024×1024**, divisibles en **16 cases de 256×256**, avec marge transparente, échantillonnage nearest et point d'ancrage local **(128, 240)**. Les sprites ont un facteur de 2 pixels par unité Unreal. Chaque rangée correspond à sud, ouest, est, nord. La taille des monstres est volontairement inférieure à celle du héros.

Pour préparer de nouvelles sources, depuis la racine du dépôt :

```sh
python -m pip install -r Tools/requirements-art.txt
python Tools/prepare_art.py
python Tests/check_art.py
```

Modifier le catalogue avant une régénération. Le manifeste contient les SHA-256 des originaux et des atlas. Un manifeste modifié provoque la reconstruction des assets au prochain lancement. Pour forcer une reconstruction sans modifier les images, fermer l'éditeur et supprimer seulement `Saved/FinalTowerArt/imported.txt`.

## Vérifications à effectuer dans UE et sur téléphone

- Premier lancement : import complet sans erreur, héros visible, aucune forme de secours restante.
- ZQSD/WASD et glissement : marche dans quatre directions ; retour au repos dans la dernière direction.
- Caméra et sol : sprites tournés vers la caméra, pieds ancrés sans clipping majeur avec le décor.
- Mousse visible près du héros ; Luciolle et Roncinet visibles à leurs positions, animés et interactifs.
- Attaquer et recruter : gestes du héros joués ; Roncinet recruté avec un message adapté, sans déclencher la quête de Luciolle.
- Changer de compagnon : visuel mis à jour ; créature recrutée retirée de son point de rencontre.
- Carnet : cinq pages, navigation et fermeture ; chaque aperçu du héros se termine après trois secondes et rend le contrôle.
- Arrêter puis relancer : pas de second import si rien n'a changé ; sauvegarde du roster conservée.
- Build Development Android : assets présents après cuisson, transparence correcte, mémoire et temps d'image mesurés.

**Limite de validation actuelle :** les images, leur découpage, les manifestes, les chemins et les tests autonomes ont été vérifiés ici. Unreal Editor/UnrealBuildTool et le SDK mobile ne sont pas installés dans cet environnement. La compilation du nouveau module Editor, la fabrication des `.uasset`, le rendu en jeu et le packaging restent à exécuter sur une machine équipée. Aucun APK n'a été produit.
