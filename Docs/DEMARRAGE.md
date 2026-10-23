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

## Menu principal (0.6)

Au lancement de Play, le menu principal apparaît au-dessus de la scène. Son fond bleu nuit, son emblème de tour, ses cadres dorés et ses boutons sont dessinés en code, sans nouvelles images ni import d'assets.

- **Continuer** charge le slot existant et ses migrations. Il est grisé si aucun fichier valide n'est disponible.
- **Nouvelle partie** commence avec Mousse, 3 baies, 2 rosées et 2 friandises. Si un fichier existe, même illisible, une confirmation est obligatoire. **Annuler** est le choix sélectionné par défaut.
- Souris ou tactile : toucher le bouton. Clavier : Haut/Bas ou Tab, puis Entrée ou Espace ; Échap annule la confirmation (dans PIE, l'éditeur peut intercepter Échap pour arrêter Play).
- Rien n'est écrit lors de l'ouverture du menu ou de l'annulation. Les déplacements, interactions et raccourcis du jeu sont bloqués jusqu'au choix d'une partie.
- Une erreur de chargement ou de création garde le menu affiché avec un message. La nouvelle partie est écrite avant d'appliquer son état en mémoire, sans suppression préalable du slot.

Ce lot couvre uniquement l'écran de démarrage. Les menus du sac, de l'équipe et du journal gardent leur présentation existante.

Vérifications à effectuer **dans Unreal**, après compilation :

1. Dans un profil de test sans sauvegarde, ouvrir Play : Continuer grisé ; aucun fichier créé avant Nouvelle partie. Tester I, J, E, Tab et un glissement hors bouton : aucun accès au jeu.
2. Choisir Nouvelle partie : entrée dans la scène, un seul Mousse, sac de départ, quêtes et découvertes à zéro. Maintenir le doigt après le clic : pas de déplacement involontaire.
3. Modifier la progression, sauvegarder, arrêter et relancer Play : Continuer actif. Le choisir et retrouver compagnons, inventaire, quêtes et découvertes, sans nouvelle distribution d'objets.
4. Relancer Play, choisir Nouvelle partie puis Annuler : la sauvegarde reste inchangée ; Continuer retrouve la même partie.
5. Relancer Play, choisir Nouvelle partie puis Recommencer : vérifier le retour à zéro de l'équipe, du sac et des quêtes. Double-clic rapide : une seule création.
6. Avec une copie de test illisible/incompatible : Continuer grisé ; Nouvelle partie demande toujours confirmation. Avec un dossier de sauvegarde non inscriptible : rester au menu avec un message d'erreur.
7. Tester les anciennes sauvegardes v1–v3 avec Continuer ; vérifier la migration et les compagnons conservés.
8. Vérifier cadrage, contraste, textes et cibles tactiles en paysage 16:9, 16:10, 4:3 et sur téléphone. La compilation Unreal, le rendu Canvas et les écritures réelles du slot restent à valider ; les tests portables ne les exécutent pas.

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
| Journal des quêtes | Bouton Journal | J ouvre et ferme le journal |
| Techniques en combat | Trois boutons de techniques | 4 / 5 / 6 |
| Sauvegarder | Bouton Sauver, hors dialogue/combat | — |

Un seul doigt est géré dans ce socle. La souris fonctionne sans activer Use Mouse for Touch. L'interface Canvas native sert au prototype ; une interface UMG avec zones sûres, accessibilité et navigation complète est un jalon ultérieur.

## Parcours de vérification dans Unreal

Ces vérifications **restent à exécuter dans le moteur** :

1. Compiler la cible Editor sans erreur UHT ni erreur C++.
2. Lancer Play, choisir Nouvelle partie ou Continuer ; vérifier le sol, les formes, la caméra, la lumière et les textes.
3. Marcher au clavier et par glissement de souris ; confirmer les collisions avec la maison et les limites de zone.
4. Approcher la gardienne ; dérouler ses répliques. Vérifier que le héros s'arrête pendant le dialogue.
5. Rejoindre la créature ; vérifier que les déplacements sont suspendus pendant le combat.
6. Attaquer trois fois avec Mousse (la créature passe de 30 à 9 PV), puis tenter l'amitié. La réussite reste aléatoire ; réessayer si nécessaire.
7. Vérifier qu'une réussite ajoute un seul compagnon, masque la créature dans le monde et met à jour l'objectif.
8. Retourner parler à la gardienne ; terminer le dialogue et vérifier l'objectif final.
9. Changer de compagnon, sauver, arrêter puis relancer Play et choisir Continuer : collection, PV, compagnon actif et histoire doivent être restaurés. La position revient au centre.
10. Sur une nouvelle sauvegarde, tester fuite, victoire sans recrutement et défaite ; la rencontre doit rester disponible.
11. Vérifier qu'un clic/toucher n'active pas deux fois une action et qu'une action d'un combat terminé ne donne pas un second compagnon.

Les sauvegardes utilisent le slot `FinalTower_Prototype_v1`. Pour reprendre à zéro, relancer Play, choisir **Nouvelle partie**, puis confirmer **Recommencer**. Les sessions PIE partagent ce slot : éviter plusieurs instances simultanées pour ce prototype.

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

## Exploration et journal (0.5)

Les nouveaux coffres, plantes et nid sont des formes géométriques dans la zone de test. La Mousse de quête réutilise son sprite idle ; une animation spécifique de blessure reste à créer. Le journal s’ouvre avec **J** ou **Journal** et montre les deux histoires, leurs indices et leurs récompenses. Les menus bloquent les déplacements.

Parcours supplémentaire à vérifier dans Unreal :

1. Ouvrir le coffre à l’est de la maison et récolter le buisson à l’ouest. Vérifier les quantités, puis sauvegarder et relancer : le coffre reste ouvert, la plante disparaît et les objets ne sont pas redonnés.
2. À l’ouest, rencontrer la petite Mousse. Terminer chaque dialogue. Réinteragir pour utiliser une baie, puis une friandise ; les invites affichent les coûts et le sac doit perdre exactement une unité de chaque objet.
3. Chercher le nid au nord-ouest et lire le journal. Mousse doit maintenant suivre le héros. Le nid peut aussi être fouillé avant la première rencontre sans bloquer la quête.
4. Retourner voir la gardienne et terminer ses trois répliques : une Mousse niveau 2 rejoint la collection et deux rosées de lune sont ajoutées. Une équipe déjà complète envoie le nouveau compagnon en réserve.
5. Relancer le jeu, puis reparler à la gardienne : aucun nouveau compagnon ni objet de quête ne doit apparaître. La quête de la lanterne reste accessible indépendamment de celle de Mousse.
6. Essayer chaque étape sans l’objet demandé, puis revenir après récolte ou victoire. Vérifier qu’aucune étape n’avance sans consommer son objet.
7. Essayer un coffre avec une pile presque pleine : aucun butin partiel ni changement de découverte. Libérer assez de place et revenir. Même contrôle pour les deux rosées de récompense finale.
8. Charger une sauvegarde V3 : compagnons et quantités inchangés, découvertes et quête de Mousse disponibles. Reprendre après une sauvegarde à chacune des étapes de la quête.
9. Avec deux cibles à portée, vérifier que l’invite et l’action concernent la cible la plus proche ; vérifier les six boutons en paysage tactile, J et I, et la lecture complète des dialogues.

Ces scénarios dans le moteur restent à exécuter. Les règles indépendantes d’Unreal sont testées automatiquement. Voir [EXPLORATION.md](EXPLORATION.md) pour les emplacements et les sauvegardes.

