# Progression et équipe — prototype 0.4

## Boucle jouable prévue par le code

Rencontrer un monstre → utiliser attaques, techniques ou amitié → recevoir de l'XP en cas de victoire ou de recrutement → gagner des niveaux et apprendre des techniques → ajuster l'équipe et la réserve.

Les règles sont testables sans le moteur dans `FTProgressionRules.h` et `FTBattleRules.h`. L'adaptateur Unreal les utilise dans les combats, la collection et les sauvegardes. Le rendu de l'interface et sa compilation Unreal restent à vérifier sur une installation équipée.

## Niveaux et XP

Le plafond provisoire est le **niveau 20**. Le seuil cumulé du niveau N est `10 × N × (N − 1)` :

| Niveau | XP cumulée |
|---|---:|
| 1 | 0 |
| 2 | 20 |
| 3 | 60 |
| 4 | 120 |
| 5 | 200 |
| 20 | 3 800 |

Un combat gagné ou un recrutement accorde sa récompense complète à chaque compagnon **ayant participé et encore debout**. La réserve et le monstre venant d'être recruté ne reçoivent pas cette récompense. Fuite et défaite ne donnent pas d'XP. Une rencontre terminée ne peut pas donner sa récompense deux fois.

Récompense de base : Mousse 16 XP, Luciolle 24 XP, Roncinet 30 XP ; ajouter 8 XP par niveau ennemi au-delà du premier. Les deux rencontres de la zone de test ont le niveau du compagnon actif, plafonné à 3. Elles réapparaissent **12 secondes après une victoire ou un recrutement**, même si l'espèce figure déjà dans la collection. Cela permet d'entraîner l'équipe et de recruter plusieurs individus ; ce n'est pas encore un système de population du monde.

À un gain de niveau, les PV et PM actuels augmentent seulement de l'augmentation de leurs maximums. Cela ne remplit pas complètement les jauges et ne ranime pas un compagnon K.-O. La gardienne restaure les PV et PM de toute la collection, réserve comprise. Une défaite totale conserve le comportement de secours du prototype : repos et récupération complète, sans XP.

## Statistiques

Les statistiques du niveau 1 sont complétées par une croissance fixe à chaque niveau :

| Espèce | PV | Attaque | Défense | PM | Magie |
|---|---|---|---|---|---|
| Mousse | 40 (+5) | 8 (+2) | 2 (+2) | 10 (+2) | 7 (+2) |
| Roncinet | 38 (+6) | 7 (+3) | 3 (+2) | 6 (+1) | 3 (+1) |
| Luciolle | 30 (+4) | 6 (+1) | 1 (+1) | 12 (+3) | 9 (+3) |

Le niveau est dérivé de l'XP, ce qui évite un désaccord entre deux valeurs sauvegardées. Un monstre recruté rejoint la collection au niveau de la rencontre, avec ses PV restants et ses PM au maximum. Les individus ont des identifiants distincts ; on peut posséder plusieurs membres d'une même espèce.

## Neuf techniques

| Espèce | Niveau | Technique | PM | Effet |
|---|---:|---|---:|---|
| Mousse | 1 | Pousse vitale | 3 | Soin du compagnon actif : 8 + Magie |
| Mousse | 3 | Écorce | 3 | Défense +5 pendant 3 ripostes |
| Mousse | 5 | Souffle vert | 5 | Attaque magique, puissance 8 |
| Roncinet | 1 | Charge | 2 | Attaque physique, puissance 5 |
| Roncinet | 3 | Enracinement | 3 | Défense +7 pendant 2 ripostes |
| Roncinet | 5 | Brise-roche | 5 | Attaque physique, puissance 12 |
| Luciolle | 1 | Étincelle | 3 | Attaque magique, puissance 4 |
| Luciolle | 3 | Éblouissement | 3 | Attaque ennemie −4 pendant 3 ripostes |
| Luciolle | 5 | Lueur apaisante | 4 | Soin du compagnon actif : 12 + Magie |

Attaque physique : `max(1, Attaque + puissance − Défense ennemie)`. Une attaque normale a une puissance de 0. Attaque magique : `max(1, Magie + puissance − moitié entière de la Défense ennemie)`. Les noms nature/lumière définissent ici la présentation ; il n'existe pas encore de tableau de faiblesses élémentaires.

Une technique valide consomme des PM puis donne une riposte à l'ennemi s'il est toujours debout. Une technique verrouillée, étrangère à l'espèce, sans assez de PM ou un soin à PV pleins ne consomme ni PM ni tour. Une protection s'applique dès la riposte suivant son lancement. La relancer la rafraîchit sans additionner ses valeurs. L'affaiblissement ennemi persiste lors d'un relais ; la protection personnelle ne passe pas au nouveau compagnon.

Les techniques connues sont déduites de l'espèce et du niveau. Un message annonce les nouvelles techniques au passage de niveau. Les ennemis utilisent encore leurs attaques normales ; leur IA de compétences n'est pas incluse dans ce jalon.

## Équipe et réserve

- Jusqu'à **trois membres** dans l'équipe ; les recrutements supplémentaires rejoignent la réserve.
- L'écran **Équipe** permet de choisir le compagnon en tête et d'ajouter/retirer des membres hors combat.
- L'équipe doit conserver au moins un membre ; le compagnon en tête doit être en état de combattre.
- Pendant un combat, seul un membre vivant de l'équipe peut entrer : aucun accès direct à la réserve.
- Un relais volontaire donne immédiatement une riposte à l'ennemi. Un remplacement après K.-O. est gratuit.
- Après un K.-O., le choix d'un remplaçant s'ouvre automatiquement. Les autres actions restent bloquées jusqu'au choix. Si toute l'équipe est K.-O., la rencontre se termine en défaite.
- Chaque compagnon conserve ses PV et PM entre ses interventions. Les membres ayant réellement participé sont suivis par leur identifiant, même après plusieurs relais.
- La collection du prototype est limitée à 1 000 individus, pour rester compatible avec la validation des sauvegardes.

## Contrôles

| Action | Tactile / souris | Clavier |
|---|---|---|
| Équipe / réserve | Bouton Équipe | Tab |
| Attaque normale | Bouton Attaquer | 1 |
| Amitié | Bouton Amitié | 2 |
| Fuite | Bouton Reculer | 3 |
| Techniques 1 / 2 / 3 | Trois boutons affichant coût ou niveau requis | 4 / 5 / 6 |
| Relais en combat | Équipe puis Relayer | Tab puis choix à l'écran |

Les boutons indisponibles sont grisés. Le menu Équipe présente niveau, PV/PM, XP, statistiques, rôle et trois techniques de chaque individu, avec pagination de la réserve. Il bloque les déplacements pendant sa consultation.

## Compatibilité des sauvegardes

Le slot existant `FinalTower_Prototype_v1` est conservé ; le schéma interne est maintenant **3**, avec l’inventaire ; les versions 1 et 2 restent lisibles. Voir [INVENTAIRE.md](INVENTAIRE.md). Une sauvegarde du prototype 0.1/0.2 conserve ses identifiants de monstres, espèces, PV, compagnon actif et progression de quête. Les monstres commencent au niveau 1 avec leurs PM initialisés ; une équipe de trois maximum est créée en conservant l'ancien compagnon actif.

Les nouvelles sauvegardes conservent XP, PV, PM, membres d’équipe, réserve, compagnon actif et quantités d’objets. Les indices invalides ou doublons d'équipe sont retirés au chargement ; l'XP et les ressources sont bornées. Les espèces inconnues et identifiants invalides/doublons sont rejetés avant de remplacer la session. Une rencontre ou un dialogue en cours ne se sauvegarde pas.

## Validation

Les tests autonomes couvrent les seuils exacts, le plafond et les gros gains d'XP, plusieurs niveaux d'un coup, les techniques par espèce, leurs coûts, soins/protections/affaiblissements, les relais payants/gratuits, les actions après la fin du combat, la normalisation des équipes et un entraînement complet jusqu'au niveau 5. Les anciennes règles de combat, les directions des sprites et les atlas restent testés.

À effectuer dans Unreal : compilation/UHT ; affichage et commandes mobiles ; recrutement d'au moins quatre individus pour exercer la réserve ; sauvegarde et relance ; chargement d'une ancienne sauvegarde ; K.-O. avec remplaçant et défaite de toute l'équipe ; temporisation de réapparition. Aucun test Unreal ou sur téléphone n'a été exécuté ici.
