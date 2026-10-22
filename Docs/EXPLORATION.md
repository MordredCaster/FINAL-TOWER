# Exploration et histoires — prototype 0.5

La zone de test comporte cinq découvertes et une nouvelle Mousse liée à une histoire. Les objets du monde utilisent encore les formes géométriques du prototype ; la Mousse réutilise les sprites existants. Cette mise à jour ajoute le comportement, les dialogues, le journal et la sauvegarde. Le rendu, les interactions tactiles et la compilation Unreal restent à vérifier dans le moteur.

## Interagir et consulter le journal

Approcher une cible et toucher **Interagir**, ou presser **E/Espace**. Dans un rayon de 145 unités, la cible la plus proche est choisie ; l’invite au-dessus de la scène annonce la même action. Le soin et la friandise de quête affichent leur coût avant interaction. En dialogue, le même bouton passe à la réplique suivante.

**Journal** ou **J** ouvre les deux histoires. Chaque fiche montre son état, l’objectif courant, les indices connus et la récompense. Une histoire non commencée présente une piste d’exploration. Le compteur indique les cinq découvertes récupérées. Les objectifs et indices sont dérivés de la progression sauvegardée, sans deuxième copie à synchroniser.

Le journal, le sac et l’équipe suspendent les déplacements et les actions de combat au clavier. J et I ouvrent/ferment leur menu ou permettent de passer de l’un à l’autre. Aucun menu ne peut interrompre un dialogue ou le choix obligatoire d’un remplaçant après K.-O. Le journal peut être consulté pendant un combat sans dépenser de tour.

## Découvertes de la zone

Les directions ci-dessous utilisent les axes du monde de la zone ; la caméra est inclinée et tournée de −45°. Les coordonnées permettent de vérifier les emplacements dans l’éditeur.

| Découverte | Emplacement X/Y | Contenu |
|---|---|---|
| Coffre du village | 80 / −360, à l’est de la maison | 2 baies, 1 friandise |
| Coffre sous les pins | −530 / 340, au nord-ouest | 1 rosée, 2 friandises |
| Buisson de braise | −430 / 80, à l’ouest | 2 baies |
| Fleurs de lune | 120 / 470, au nord | 2 rosées |
| Nid sous les branches | −280 / 500, au nord-ouest | Indice : ruban et petit bateau portant le nom « Lina » |

Chaque découverte est récupérable **une seule fois par partie**. Les plantes récoltées disparaissent ; les coffres ouverts restent visibles sous une forme aplatie. Le nid reste visible et son indice est conservé dans le journal. Aucune découverte ne repousse avec le temps.

Le contenu est attribué en une seule opération. Si une pile manque de place pour recevoir tout le contenu, aucun objet n’est donné et la découverte reste disponible. Il faut utiliser assez d’objets puis revenir. Les indices ne prennent pas de place dans le sac. Une interaction pendant un combat ou un dialogue ne peut pas récolter de découverte.

## Celui qui attend sous les branches

La Mousse de quête se trouve à **−510 / −180**, à l’ouest du village. C’est un individu distinct du compagnon de départ et des rencontres de combat. Son histoire peut se dérouler avant ou après celle de la lanterne.

1. **Rencontrer Mousse.** Deux répliques révèlent sa blessure et un fil de ruban. Aucun objet n’est consommé à cette première rencontre.
2. **Soigner sa patte.** Interagir de nouveau consomme exactement **une baie de braise**. Si le sac n’en contient pas, l’étape reste inchangée et une piste indique où en trouver.
3. **Gagner sa confiance.** Une autre interaction consomme **une friandise des bois**. Mousse indique le nid au nord-ouest.
4. **Comprendre son attente.** Fouiller le nid révèle le ruban et le bateau gravé. On peut trouver cet indice avant n’importe laquelle des étapes précédentes. Lorsque Mousse est soignée, rassurée et que l’indice est trouvé, elle suit le héros.
5. **Retourner chez la gardienne.** La gardienne reconnaît les affaires de sa fille Lina. Après trois répliques, Mousse accepte de quitter l’attente et rejoint la collection au **niveau 2** ; **deux rosées de lune** sont offertes.

Le nouveau compagnon rejoint l’équipe si elle a moins de trois membres, sinon la réserve. Il ne remplace pas le compagnon actif. La récompense entière est unique et n’est donnée qu’à la fin du dialogue. Si la pile de rosées n’a pas deux places libres, l’histoire attend la prochaine visite sans récompense partielle. La collection doit aussi avoir moins de 1 000 individus, comme pour un recrutement ordinaire.

Les trois premières étapes enregistrent leur résultat à la fin de leur dialogue. Quitter avant la fin reprend le dernier état sauvegardé, sans perte persistante d’objet. Une fois confiante et renseignée, Mousse suit le héros dès la reprise ; une fois la quête terminée, la Mousse du monde disparaît et le compagnon reste dans la collection. Elle n’est pas attaquable et aucun combat ne remplace les soins de cette histoire.

## La lanterne du passeur

La quête existante reste indépendante. Parler à la gardienne, recruter Luciolle puis revenir termine son histoire. Les dialogues avec la Mousse n’activent pas les drapeaux de la lanterne. Si les deux histoires attendent une conclusion chez la gardienne, elle reçoit d’abord la Mousse ; lui reparler permet ensuite de terminer la lanterne.

Le bandeau suit en priorité la quête de Mousse lorsqu’elle est commencée et non terminée. Le journal permet toujours de lire l’objectif de l’autre histoire. Une fois les deux terminées, le bandeau propose de finir les découvertes. Il n’y a pas encore de sélection manuelle de quête suivie, de carte ou de flèche de navigation.

## Sauvegardes et code

Le slot `FinalTower_Prototype_v1` est conservé. Le schéma **4** ajoute une liste de clés de découvertes et une étape de quête (`0` inconnue, `1` rencontrée, `2` soignée, `3` confiante, `4` récompense reçue). La présence de l’indice dans les découvertes détermine si l’étape 3 permet le retour. Une quête déjà récompensée reste terminée même si son drapeau d’indice est absent d’un fichier incohérent ; elle ne peut pas redonner la récompense.

Les versions 1 à 3 sont lisibles : elles démarrent les nouvelles découvertes et la quête sans toucher à leur ancienne progression. La migration existante du sac s’applique seulement aux versions 1 et 2 ; une sauvegarde V3 conserve exactement ses quantités. Chaque découverte sauvegarde immédiatement son état et le sac. La récompense de quête sauvegarde ensemble les objets, le nouveau compagnon et l’étape finale. Un échec de sauvegarde est annoncé ; **Sauver** permet de réessayer hors dialogue/combat.

`FTExplorationRules.h` contient le catalogue, les positions, les contenus, les étapes de quête et les règles de migration. `UFTAdventureSubsystem` expose la collecte, le dialogue et les fiches du journal aux Blueprints. Le mode de jeu gère les représentations, la portée et le choix de la cible ; le HUD affiche le journal et les invites.

```sh
g++ -std=c++17 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -I Source/FinalTower/Public Tests/exploration_rules_test.cpp -o /tmp/finaltower-exploration
/tmp/finaltower-exploration
```

Les tests couvrent les butins sans perte ni doublon, la place dans le sac, les objets requis, la quête complète, un indice trouvé à l’avance, la reprise des états intermédiaires, la récompense unique et les migrations V1–V4. Ils vérifient aussi que les découvertes sont dans les limites de la zone, hors de la maison solide. Ils ne compilent pas Unreal et ne vérifient pas sa sérialisation ni les collisions en jeu. Les essais à effectuer dans le moteur figurent dans [DEMARRAGE.md](DEMARRAGE.md).
