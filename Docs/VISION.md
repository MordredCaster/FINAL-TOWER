# Direction de FINAL TOWER

## Ce qui est décidé

- Nouveau projet, indépendant du jeu de runes.
- Mobile, Unreal Engine 5.8, mélange 2D/3D.
- Trois piliers : capture/recrutement de monstres, exploration, histoires touchantes.
- Sensation recherchée : partir à l'aventure et s'attacher à ses rencontres.
- Références : Dragon Quest V pour l'aventure, Octopath Traveler et Tactics Ogre: Reborn pour la présentation visuelle.
- Budget limité : périmètre court, décors réutilisables et identité visuelle cohérente.

## Hypothèses du prototype — modifiables

| Choix de départ | Intention |
|---|---|
| Solo hors ligne | Se concentrer sur une aventure et une sauvegarde locale |
| Écran paysage | Garder le décor et les commandes de combat lisibles |
| Tour par tour, un compagnon contre un monstre | Tester l'attachement et le recrutement avec peu d'animations |
| Une créature visible avant le combat | Laisser le joueur choisir sa rencontre |
| Caméra inclinée fixe | Préparer des sprites 2D dans des volumes simples |
| Android comme premier appareil de test | Valider rapidement les commandes et le coût du rendu ; iOS ensuite |

Ces choix ne constituent pas une validation définitive du gameplay ni des plateformes de sortie. La référence à Tactics Ogre concerne l'esthétique ; une grille tactique n'est pas imposée.

## Boucle visée

Explorer un lieu → rencontrer un personnage ou découvrir une piste → rencontrer une créature → combattre et gagner sa confiance → employer ses talents ou retourner voir un habitant → découvrir un nouveau lieu.

Dans cette première version, seuls le combat, l'amitié, les dialogues et la collection sont codés. Les talents d'exploration des monstres viendront ensuite.

## Le recrutement

Le premier prototype propose l'action **Tendre la main**. Une créature vivante à pleine santé a 15 % de chances d'accepter. La probabilité augmente avec les PV manquants et reste inférieure à 80 % tant qu'elle est vivante. Une tentative ratée provoque une riposte ; une réussite termine le combat et ajoute une instance de la créature à la collection.

Les pourcentages sont provisoires. Une version ultérieure pourra ajouter nourriture, personnalité et quêtes de rencontre. Aucun achat ou tirage payant n'est prévu dans ce prototype.

## Première histoire proposée : La lanterne du passeur

Une gardienne entretient une lanterne éteinte depuis le départ de sa fille. Une créature lumineuse, aperçue près des roseaux, fredonne un air familier. Le joueur peut l'apprivoiser et revenir auprès de la gardienne. La lumière ravive un souvenir et la lanterne s'allume.

Il s'agit d'une scène de test originale, remplaçable. L'issue reste volontairement ouverte : le souvenir compte davantage qu'une longue explication. La créature reste auprès du joueur à la fin.

## Direction artistique

- Silhouettes lisibles et petits sprites expressifs ; portraits réservés aux dialogues importants.
- Décors 3D simples, textures cohérentes, caméra contrôlée et points d'intérêt clairs.
- Palette chaude près des habitations, fraîche près de l'eau ; contrastes concentrés sur les lieux utiles.
- Effets mobiles mesurés : quelques particules et animations d'eau plutôt qu'une accumulation d'effets coûteux.
- Aucun objectif de reproduire la qualité de production complète d'Octopath avec ce premier socle.

Le code active le rendu mobile Forward et désactive Lumen, Nanite, le flou de mouvement et le HDR mobile comme point de départ prudent. Le rendu final et ses effets seront choisis après mesure sur un appareil réel.

## Étapes

1. Valider compilation, entrée tactile, combat, recrutement et sauvegarde dans Unreal.
2. Tester sur un Android réel et fixer un appareil de référence, une fréquence d'images et un budget mémoire.
3. Produire un premier héros 2D animé, deux créatures, une gardienne et un petit décor original.
4. Ajouter une capacité d'exploration donnée par un monstre, puis une seconde histoire.
5. Étendre le monde uniquement après validation de cette tranche jouable.

## À décider ensemble

L'univers et son ton, le rôle de FINAL TOWER dans l'histoire, les compagnons humains, la taille de l'équipe en combat, la progression, l'éventuelle évolution des monstres et la durée de l'aventure.
