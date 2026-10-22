# Démarrage et vérification

## Prérequis

- Unreal Engine **5.8**, avec le composant de plateforme Android si vous ciblez Android.
- Outils C++ pris en charge par votre installation Unreal. Sous Windows, installer les composants C++/jeu pour Visual Studio recommandés par Epic pour 5.8.
- Pour Android : SDK, NDK et Java dans les versions demandées par l'outil de configuration/Turnkey de votre installation 5.8. Ne pas choisir les versions au hasard.
- Pour iOS : un Mac, Xcode compatible et la signature Apple requise pour le déploiement.
- Git et Git LFS pour les futurs fichiers binaires.

Documentation Epic : [configuration Android](https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-android-sdk-and-ndk-for-unreal), [rendu mobile](https://dev.epicgames.com/documentation/en-us/unreal-engine/mobile-rendering-features-in-unreal-engine), [Paper2D](https://dev.epicgames.com/documentation/en-us/unreal-engine/paper-2d-in-unreal-engine).

## Ouvrir

Ouvrir `FinalTower.uproject`. Si les modules ne peuvent pas être compilés automatiquement, générer les fichiers de projet et compiler `FinalTowerEditor`, configuration Development Editor. Ouvrir ensuite le projet et lancer Play en mode Selected Viewport ou New Editor Window.

Au premier lancement, attendre la fin de l'import automatique décrit dans [ART.md](ART.md). Le héros et les monstres utilisent les sprites préparés ; les planches de chara design sont accessibles avec le bouton **Carnet**.

La scène géométrique est construite par `AFTPrototypeGameMode::BeginPlay` dans la carte moteur `/Engine/Maps/Entry`. Les BasicShapes sont incluses explicitement dans la cuisson. Les paramètres de projet choisissent ce mode de jeu globalement. Si une carte de travail est créée ultérieurement, vérifier qu'elle conserve ce mode ou son remplaçant.

## Commandes

| Action | Téléphone / souris | Clavier |
|---|---|---|
| Marcher | Maintenir hors des boutons, puis glisser dans la direction voulue | ZQSD, WASD ou flèches |
| Interagir à proximité | Bouton Interagir | E ou Espace |
| Lire la réplique suivante | Bouton Suite | E ou Espace |
| Attaquer en combat | Bouton Attaquer | 1 |
| Recruter en combat | Bouton Tendre la main | 2 |
| Quitter le combat | Bouton Reculer | 3 |
| Équipe et réserve / relais en combat | Bouton Équipe | Tab |
| Inventaire | Bouton Sac, puis objet / cible | I ouvre et ferme le sac |
| Techniques en combat | Trois boutons de techniques | 4 / 5 / 6 |
| Sauvegarder | Bouton Sauver, hors dialogue/combat | — |

Un seul doigt est géré dans ce socle. La souris fonctionne sans activer Use Mouse for Touch. L'interface Canvas native sert au prototype ; une interface UMG avec zones sûres, accessibilité et navigation complète est un jalon ultérieur.

## Parcours de vérification dans Unreal

Ces vérifications **restent à exécuter dans le moteur** :

1. Compiler la cible Editor sans erreur UHT ni erreur C++.
2. Lancer la scène ; vérifier le sol, les formes, la caméra, la lumière et les textes.
3. Marcher au clavier et par glissement de souris ; confirmer les collisions avec la maison et les limites de zone.
4. Approcher la gardienne ; dérouler ses répliques. Vérifier que le héros s'arrête pendant le dialogue.
5. Rejoindre la créature ; vérifier que les déplacements sont suspendus pendant le combat.
6. Attaquer trois fois avec Mousse (la créature passe de 30 à 9 PV), puis tenter l'amitié. La réussite reste aléatoire ; réessayer si nécessaire.
7. Vérifier qu'une réussite ajoute un seul compagnon, masque la créature dans le monde et met à jour l'objectif.
8. Retourner parler à la gardienne ; terminer le dialogue et vérifier l'objectif final.
9. Changer de compagnon, sauver, arrêter puis relancer Play : collection, PV, compagnon actif et histoire doivent être restaurés. La position revient au centre.
10. Sur une nouvelle sauvegarde, tester fuite, victoire sans recrutement et défaite ; la rencontre doit rester disponible.
11. Vérifier qu'un clic/toucher n'active pas deux fois une action et qu'une action d'un combat terminé ne donne pas un second compagnon.

Les sauvegardes utilisent le slot `FinalTower_Prototype_v1`. Pour reprendre à zéro dans l'éditeur, arrêter le jeu puis supprimer uniquement son fichier de sauvegarde local dans `Saved/SaveGames/`. Les sessions PIE partagent ce slot : éviter plusieurs instances simultanées pour ce prototype.

## Sur téléphone

1. Commencer avec un build **Development Android**, arm64, sur un téléphone connecté.
2. Configurer le SDK/NDK via les instructions de l'installation 5.8 ; vérifier les réglages de package/signature avant diffusion.
3. Packager ou utiliser Launch On, puis refaire le parcours avec le doigt.
4. Vérifier affichage paysage, textes, zones touchables et sauvegarde après fermeture complète.
5. Mesurer temps d'image, mémoire, chauffe et durée de lancement. Mobile Preview est utile, mais ne remplace pas ce test.

Ni compilation Unreal, ni cuisson, ni installation Android/iOS n'ont été exécutées dans l'environnement de création de ce dépôt.

## Personnaliser les graphismes

Les images actuelles sont déjà reliées au code et importées automatiquement. Voir [ART.md](ART.md) pour leur usage, le découpage, les chemins et les vérifications. Pour remplacer un visuel, modifier les sources dans `ArtSource/` et régénérer les atlas avant de rouvrir l'éditeur. Les animations de marche des monstres et les sprites du frère restent à créer.

## Progression et équipe (0.3)

Le bouton Équipe ou Tab ouvre la gestion des trois membres actifs et de la réserve. En combat, les touches 4/5/6 lancent les techniques apprises ; les choix sans PM sont désactivés. Après un K.-O., choisir un remplaçant avant de continuer. Les rencontres de la zone de test réapparaissent après 12 secondes. Voir [PROGRESSION.md](PROGRESSION.md) pour les règles, la migration des sauvegardes et les contrôles.

## Objets et inventaire (0.4)

Le bouton **Sac** ou la touche **I** ouvre et ferme le sac. Les baies rendent 25 PV, les rosées 8 PM, et les friandises ajoutent 20 points à la probabilité de recrutement pour la rencontre. Une seule friandise peut être donnée par rencontre. En combat, utiliser un objet provoque une riposte ; ouvrir/fermer le sac ne prend aucun tour. Hors combat, les soins peuvent cibler la réserve et sont immédiatement sauvegardés. Le déplacement et les autres raccourcis sont suspendus dans le sac.

À vérifier dans Unreal après compilation :

1. Nouveau départ : 3 baies, 2 rosées, 2 friandises ; tous les objets et quantités sont lisibles en paysage.
2. Commencer un combat avec des PV manquants ; utiliser une baie, constater le soin plafonné puis une seule riposte, et une quantité en moins.
3. Dépenser des PM puis utiliser une rosée ; vérifier que le bouton est grisé si les PM sont pleins ou la pile vide.
4. Donner une friandise : la chance affichée augmente de 20 points (plafond 95 %), une riposte se produit, une deuxième friandise est refusée. Relayer un compagnon : le bonus reste actif.
5. Gagner contre Roncinet : +1 baie et +1 friandise ; contre Luciolle : +1 rosée. Recruter ou fuir ne donne pas d’objet.
6. Hors combat, soigner un membre de réserve ; vérifier sa quantité et ses ressources après relance. Une cible K.-O. ou déjà pleine ne consomme rien. Vérifier la pagination avec plus de quatre compagnons.
7. Charger une sauvegarde V1 ou V2 : collection et progression sont conservées, le sac de départ est ajouté une fois. Consommer un objet, relancer deux fois : la quantité ne se remplit pas.
8. Finir un combat avec un sac modifié, fermer complètement puis relancer : ressources et objets doivent correspondre au même état sauvegardé. Un combat en cours n’est pas un point de sauvegarde.

Les règles de quantités, d’effets, de tours et de migration ont des tests C++ autonomes ; la sérialisation Unreal et l’interface restent à vérifier dans le moteur. Détails : [INVENTAIRE.md](INVENTAIRE.md).
