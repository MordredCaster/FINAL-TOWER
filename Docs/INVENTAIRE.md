# Objets et inventaire — prototype 0.5

Le sac est partagé par toute la collection. Le bouton **Sac** et la touche **I** l’ouvrent et le ferment, en exploration comme en combat. Trois piles de 99 objets au maximum sont prévues ; les entrées restent visibles à zéro. Le menu bloque les déplacements et les actions de combat au clavier. L’interface est le HUD Canvas natif du prototype, à vérifier dans Unreal et sur téléphone.

## Catalogue

| Objet | Identifiant de sauvegarde | Effet | Départ |
|---|---|---|---:|
| Baie de braise | `healing_berry` | Rend 25 PV, sans dépasser le maximum | 3 |
| Rosée de lune | `moon_dew` | Rend 8 PM, sans dépasser le maximum | 2 |
| Friandise des bois | `friendship_treat` | Augmente de 20 points la probabilité de recrutement pendant la rencontre | 2 |

Les soins s’appliquent uniquement à des compagnons encore debout : ces objets ne raniment pas les K.-O. Une cible dont la ressource est déjà pleine ne consomme rien. Les PM d’une cible K.-O. ne peuvent pas non plus être restaurés.

Hors combat, sélectionner un soin ouvre la liste de tous les compagnons, avec PV, PM, numéro d’individu et appartenance à l’équipe ou à la réserve. Quatre cibles sont affichées par page. Après un soin, la liste reste ouverte et les quantités sont mises à jour ; plusieurs soins peuvent ainsi être donnés sans rouvrir le sac.

En combat, les soins ciblent uniquement le compagnon actif. Un objet valide est consommé et son effet s’applique **avant une seule riposte**. Le sac se ferme ensuite pour montrer le résultat. Ouvrir ou fermer le sac ne prend aucun tour ; une action refusée ne consomme ni objet ni tour. Pendant un remplacement obligatoire après K.-O., le sac est indisponible.

## Friandises et recrutement

Une seule friandise peut être donnée par rencontre. Elle ne recrute pas immédiatement : elle améliore les tentatives **Amitié** suivantes. L’ennemi riposte après avoir reçu l’objet, comme après un soin. Le bonus persiste après un relais volontaire ou un remplacement après K.-O. ; il disparaît au début de la rencontre suivante.

La chance vaut `15 % + 65 % × proportion des PV ennemis manquants + 20 points si une friandise a été donnée`, avec un plafond de **95 %**. La valeur est affichée dans le panneau de combat, ainsi que l’état du bonus. Un ennemi non recrutable ne peut pas recevoir de friandise.

## Obtenir des objets

Le sac de départ accompagne une nouvelle partie. Les anciennes sauvegardes le reçoivent lors de leur migration. Les victoires fournissent ensuite des objets, pour permettre de poursuivre les essais sans achats :

| Victoire contre | Butin |
|---|---|
| Roncinet | 1 baie de braise et 1 friandise des bois |
| Luciolle | 1 rosée de lune |
| Mousse | 1 baie de braise, si une rencontre de cette espèce est lancée par Blueprint |

La scène actuelle propose Roncinet et Luciolle, qui réapparaissent après 12 secondes. Recrutements, fuites et défaites ne donnent aucun objet. Les récompenses sont accordées lors de la résolution de la rencontre, une seule fois. Une pile pleine reste à 99 ; le message précise les objets non ajoutés. Les coffres et les plantes de la zone fournissent aussi des objets, une fois chacun ; voir [EXPLORATION.md](EXPLORATION.md). Il n’y a pas encore de boutique, équipement, fabrication ou objet de résurrection.

Deux actions de quête utilisent le sac : soigner la Mousse blessée coûte une baie, puis la rassurer coûte une friandise. Le coût apparaît dans l’invite d’interaction et les objets sont déduits une seule fois par étape. Ce soin narratif concerne une créature du monde, pas un compagnon de la collection.

## Sauvegarde

Le slot reste `FinalTower_Prototype_v1`. Le schéma interne actuel est **4** ; la version 3 a ajouté une table d’identifiants stables et de quantités. Les noms affichés ne servent pas de clés. Au chargement, seules les clés du catalogue sont lues, les quantités sont bornées entre 0 et 99, et une entrée absente vaut zéro.

- Version 1 : migration existante des compagnons et de l’équipe, plus sac de départ.
- Version 2 : XP, PM, équipe et réserve conservés, plus sac de départ.
- Versions 3 et 4 : quantités restaurées telles quelles, y compris un sac vide ; aucun réapprovisionnement au chargement. La version 4 ajoute les découvertes et les étapes de la quête, sans modifier les piles.

Le démarrage enregistre le nouveau schéma après une migration réussie. Chaque soin hors combat sauvegarde immédiatement. Les objets utilisés pendant une rencontre sont enregistrés avec les PV/PM et le résultat à la fin du combat, comme les autres actions ; le combat en cours ne peut pas être sauvegardé. En cas de fermeture avant sa résolution, on reprend l’état sauvegardé précédent. Un échec d’écriture est annoncé et le bouton **Sauver** permet de réessayer après le combat ou le dialogue.

## Code et validation

`FTInventoryRules.h` contient le catalogue, les piles, les soins, le sac de départ, la migration des quantités et le butin. `FTBattleRules.h` contrôle les objets utilisables, la riposte et le bonus de friandise. `UFTAdventureSubsystem` relie ces règles à la collection, à la résolution des rencontres et au fichier de sauvegarde. Il expose `GetInventory`, `GetItemQuantity`, `CanUseItem` et `UseItem` aux Blueprints pour une future interface UMG.

```sh
g++ -std=c++17 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -I Source/FinalTower/Public Tests/inventory_rules_test.cpp -o /tmp/finaltower-inventory
/tmp/finaltower-inventory
```

Ces tests couvrent piles vides/pleines, grands nombres, identifiants inconnus, soins plafonnés/refusés, ressources à zéro, consommation et riposte, friandise non cumulable, relais/K.-O., fin de combat, butin et migration des quantités. Ils ne compilent pas Unreal et n’exercent pas sa sérialisation. Le [guide de démarrage](DEMARRAGE.md) donne les vérifications à exécuter dans le moteur.
