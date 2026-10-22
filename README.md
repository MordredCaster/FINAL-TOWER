# FINAL TOWER

Un RPG mobile d'aventure : **explorer, se lier d'amitié avec des monstres et vivre des histoires touchantes**.

Projet natif **Unreal Engine 5.8**, paysage, solo hors ligne. Direction visée : personnages 2D en pixel art dans des décors 3D simples, avec une caméra fixe et une lumière soignée. Dragon Quest V inspire le sentiment d'aventure ; Octopath Traveler et Tactics Ogre: Reborn sont des références visuelles, avec des personnages et un univers originaux.

## État du projet — 0.1

Premier socle de code et scène de test générée au lancement. **Ce dépôt ne contient pas encore de graphismes finaux, de carte artistique ni d'APK.** Les personnages sont représentés par des formes géométriques. La caméra est fixe en orientation et suit le héros.

Fonctionnalités implémentées dans le code :

- Déplacement au clavier, à la souris ou au doigt avec un glissement de type joystick flottant.
- Petite zone 3D, gardienne et créature avec interaction à proximité.
- Combat au tour par tour : attaquer, tenter une amitié, reculer.
- Recrutement favorisé par l'affaiblissement ; une tentative ratée provoque une riposte.
- Collection persistante et changement du compagnon actif hors combat.
- Courte quête originale : **La lanterne du passeur**.
- Sauvegarde locale des compagnons, de leurs PV, du compagnon actif et de la progression narrative.
- Composant Paper2D remplaçable par un flipbook dans un Blueprint dérivé du héros.

**Validation :** les règles de combat et de recrutement ont des tests C++ autonomes. Unreal Editor/UnrealBuildTool ne sont pas présents dans l'environnement de création : la compilation du module, l'ouverture de la scène et l'export Android/iOS restent à valider sur une machine équipée. Une réussite de la CI des règles ne prouve pas que le projet Unreal compile.

## Premier lancement

1. Installer **Unreal Engine 5.8** et les outils C++ adaptés à votre système.
2. Cloner ce dépôt. Installer Git LFS avant d'ajouter des textures, cartes ou autres assets binaires.
3. Ouvrir `FinalTower.uproject` et accepter la génération/compilation des modules ; ou générer les fichiers de projet puis compiler la cible `FinalTowerEditor` en Development Editor.
4. Dans l'éditeur, lancer **Play**. La carte de démarrage `/Engine/Maps/Entry` reçoit le mode de jeu natif, qui construit la scène de test.

Voir [le guide de démarrage](Docs/DEMARRAGE.md) pour les contrôles, les vérifications et le mobile.

## Essayer la boucle

Approcher la **Gardienne**, puis choisir **Interagir**. Lire ses trois répliques. Rejoindre la **Lumière des roseaux**, interagir et attaquer avec modération : un monstre vaincu ne peut pas être recruté durant ce combat. Choisir **Tendre la main**, puis retourner auprès de la gardienne une fois Luciolle recrutée.

Une victoire sans recrutement laisse la rencontre disponible ; la défaite soigne l'équipe ; reculer conserve les PV restants. La gardienne soigne les compagnons. La position du héros n'est pas sauvegardée : on reprend au centre de la zone.

## Organisation

| Dossier / fichier | Rôle |
|---|---|
| `FinalTower.uproject` | Projet UE 5.8, module et plugin Paper2D |
| `Config/` | Mobile, rendu, carte de démarrage et contrôles |
| `Source/FinalTower/Public/FTBattleRules.h` | Règles de combat indépendantes du moteur |
| `Source/FinalTower/*/FTAdventureSubsystem.*` | Collection, rencontres, dialogue, sauvegarde |
| `Source/FinalTower/*/FTPrototype.*` | Héros, contrôles tactiles, caméra, HUD et scène de test |
| `Docs/` | Vision, décisions provisoires et démarrage |
| `Tests/` | Tests autonomes des règles |

## Tester les règles sans Unreal

```sh
g++ -std=c++17 -Wall -Wextra -Werror -pedantic -I Source/FinalTower/Public Tests/battle_rules_test.cpp -o /tmp/finaltower-rules
/tmp/finaltower-rules
```

Ce test couvre le recrutement répété, les actions après la fin d'un combat, les limites de probabilité, la riposte, la défaite, la fuite et les dégâts minimaux. Les scénarios à exécuter dans Unreal sont décrits dans le guide.

## Prochain jalon

Compiler et jouer cette boucle dans Unreal, puis tester sur un téléphone Android avant de produire une première petite zone artistique. La portée initiale est un village, un chemin, une rencontre et une histoire courte. L'univers global, le rôle de la tour et le système de progression restent à définir ensemble.
