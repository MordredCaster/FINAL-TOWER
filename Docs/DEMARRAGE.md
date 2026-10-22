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
| Changer de compagnon actif | Bouton Compagnon, hors combat | — |
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

## Ajouter les graphismes ensuite

Créer un Blueprint dérivé de `FTExplorer`, affecter un flipbook Paper2D au composant `PixelVisual` et ajuster sa taille et son point d'ancrage. Le placeholder se masque au lancement lorsqu'un flipbook existe. Choisir ce Blueprint comme Default Pawn Class dans un mode de jeu dérivé, puis affecter ce mode dans les paramètres/cartes du projet.

Le composant tourne vers la caméra. Il n'existe pas encore de sélection d'animations par direction ni d'animation marche/repos ; cela sera ajouté avec le premier vrai sprite. Remplacer ensuite les marqueurs de PNJ et de monstre et le décor géométrique. Importer les assets dans `Content/`, avec Git LFS activé avant leur ajout. Ne pas placer de clés, keystores ou mots de passe de signature dans le dépôt.
