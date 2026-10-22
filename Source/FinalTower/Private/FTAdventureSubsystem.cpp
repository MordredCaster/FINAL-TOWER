#include "FTAdventureSubsystem.h"
#include "Kismet/GameplayStatics.h"

namespace
{
const FString SaveSlot = TEXT("FinalTower_Prototype_v1"); // Keep the existing slot; schema versions live in the save.
using namespace FinalTowerRules;

Species Kind(FName Id)
{
    if (Id == TEXT("roncinet")) return Species::Roncinet;
    if (Id == TEXT("luciolle")) return Species::Luciolle;
    return Species::Mousse;
}

FFTMonsterSpecies MakeSpecies(FName Id, const TCHAR* Name, const TCHAR* Role)
{
    const MonsterStats Base = StatsAtLevel(Kind(Id), 1);
    FFTMonsterSpecies Result;
    Result.Id = Id; Result.Name = Name; Result.Role = Role;
    Result.MaxHP = Base.MaxHP; Result.Attack = Base.Attack; Result.Defense = Base.Defense;
    Result.MaxMP = Base.MaxMP; Result.Magic = Base.Magic;
    return Result;
}

Fighter MakeFighter(const FFTMonsterStats& Stats)
{
    return {Stats.MaxHP, Stats.HP, Stats.Attack, Stats.Defense, Stats.MaxMP, Stats.MP, Stats.Magic};
}
}

void UFTAdventureSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Species = {
        MakeSpecies(TEXT("mousse"), TEXT("Mousse"), TEXT("Soutien naturel")),
        MakeSpecies(TEXT("luciolle"), TEXT("Luciolle"), TEXT("Magie lumineuse")),
        MakeSpecies(TEXT("roncinet"), TEXT("Roncinet"), TEXT("Charge et protection"))
    };
    const bool bHadSave = UGameplayStatics::DoesSaveGameExist(SaveSlot, 0);
    const bool bLoaded = LoadProgress();
    if (!bLoaded)
    {
        AddFriend(TEXT("mousse"));
        Bag = StarterInventory();
        Message = TEXT("Mousse vous accompagne. Une gardienne attend près de la lanterne.");
    }
    // Persist migrations once; loading never refills an existing inventory or discovery.
    if ((bLoaded || !bHadSave) && !SaveProgress()) Message += TEXT(" Sauvegarde impossible : réessayez avec Sauver.");
}

const FFTMonsterSpecies* UFTAdventureSubsystem::FindSpecies(FName Id) const
{
    return Species.FindByPredicate([Id](const FFTMonsterSpecies& Entry) { return Entry.Id == Id; });
}

bool UFTAdventureSubsystem::HasFriend(FName Id) const
{
    return Friends.ContainsByPredicate([Id](const FFTMonsterFriend& Entry) { return Entry.SpeciesId == Id; });
}

FFTMonsterStats UFTAdventureSubsystem::GetFriendStats(int32 Index) const
{
    FFTMonsterStats Result;
    if (!Friends.IsValidIndex(Index)) return Result;
    const FFTMonsterFriend& Friend = Friends[Index];
    Result.Experience = GrantXP(Friend.Experience, 0);
    Result.Level = LevelFromXP(Result.Experience);
    const MonsterStats Stats = StatsAtLevel(Kind(Friend.SpeciesId), Result.Level);
    Result.HP = Friend.HP; Result.MaxHP = Stats.MaxHP;
    Result.MP = Friend.MP; Result.MaxMP = Stats.MaxMP;
    Result.Attack = Stats.Attack; Result.Defense = Stats.Defense; Result.Magic = Stats.Magic;
    Result.NextLevelXP = Result.Level < MaxLevel ? XPForLevel(Result.Level + 1) : Result.Experience;
    return Result;
}

TArray<FFTMonsterTechnique> UFTAdventureSubsystem::GetTechniques(int32 Index) const
{
    TArray<FFTMonsterTechnique> Result;
    if (!Friends.IsValidIndex(Index)) return Result;
    const FinalTowerRules::Species Id = Kind(Friends[Index].SpeciesId);
    const int32 Level = GetFriendStats(Index).Level;
    for (SkillId Technique : SkillsForSpecies(Id))
    {
        const Skill& Entry = *FindSkill(Technique);
        FFTMonsterTechnique View;
        View.Name = UTF8_TO_TCHAR(Entry.Name); View.Description = UTF8_TO_TCHAR(Entry.Description);
        View.UnlockLevel = Entry.UnlockLevel; View.MPCost = Entry.MPCost;
        View.bLearned = HasLearned(Id, Level, Technique);
        View.bUsable = bInBattle && Index == ActiveFriend && !bAwaitingReplacement && Battle.CanUseSkill(Technique);
        Result.Add(View);
    }
    return Result;
}

void UFTAdventureSubsystem::AddFriend(FName Id, int32 Level, int32 HP)
{
    if (!FindSpecies(Id)) return;
    const MonsterStats Stats = StatsAtLevel(Kind(Id), Level);
    FFTMonsterFriend Friend;
    Friend.InstanceId = FGuid::NewGuid(); Friend.SpeciesId = Id;
    Friend.Experience = XPForLevel(Level);
    Friend.HP = HP < 0 ? Stats.MaxHP : Clamp(HP, 1, Stats.MaxHP);
    Friend.MP = Stats.MaxMP;
    const int32 Index = Friends.Add(Friend);
    if (Party.Num() < PartyCapacity) Party.Add(Index);
}

bool UFTAdventureSubsystem::BeginEncounter(FName Id)
{
    if (bInBattle || !Dialogue.IsEmpty() || !Friends.IsValidIndex(ActiveFriend) || !Party.Contains(ActiveFriend)) return false;
    if (!FindSpecies(Id) || Friends[ActiveFriend].HP <= 0) return false;
    EnemyId = Id;
    const FFTMonsterStats Ally = GetFriendStats(ActiveFriend);
    // Starter-area encounters remain within levels 1–3, independently of the global level cap.
    EnemyLevel = Clamp(Ally.Level, 1, 3);
    const MonsterStats Enemy = StatsAtLevel(Kind(Id), EnemyLevel);
    Battle = {};
    Battle.Ally = MakeFighter(Ally);
    Battle.AllySpecies = Kind(Friends[ActiveFriend].SpeciesId);
    Battle.AllyLevel = Ally.Level;
    Battle.Enemy = {Enemy.MaxHP, Enemy.MaxHP, Enemy.Attack, Enemy.Defense, Enemy.MaxMP, Enemy.MaxMP, Enemy.Magic};
    BattleParticipants.Reset(); BattleParticipants.Add(Friends[ActiveFriend].InstanceId);
    bAwaitingReplacement = false; bInBattle = true;
    Message = FString::Printf(TEXT("%s niv. %d vous observe. Affaiblissez-la, puis tendez la main."), *FindSpecies(Id)->Name, EnemyLevel);
    return true;
}

void UFTAdventureSubsystem::Attack()
{
    if (!bInBattle || bAwaitingReplacement || !Battle.Attack()) return;
    Message = TEXT("Votre compagnon attaque. La créature riposte si elle tient encore debout.");
    FinishAction();
}

bool UFTAdventureSubsystem::UseSkill(int32 Index)
{
    if (!bInBattle || bAwaitingReplacement || !Friends.IsValidIndex(ActiveFriend) || Index < 0 || Index >= 3) return false;
    const auto Known = SkillsForSpecies(Kind(Friends[ActiveFriend].SpeciesId));
    const Skill& Entry = *FindSkill(Known[Index]);
    if (!HasLearned(Battle.AllySpecies, Battle.AllyLevel, Entry.Id))
    {
        Message = FString::Printf(TEXT("%s s'apprend au niveau %d."), UTF8_TO_TCHAR(Entry.Name), Entry.UnlockLevel);
        return false;
    }
    if (Battle.Ally.MP < Entry.MPCost) { Message = TEXT("Pas assez de PM. La gardienne peut vous reposer."); return false; }
    if (!Battle.UseSkill(Entry.Id)) { Message = TEXT("Ce soin est inutile : les PV sont déjà au maximum."); return false; }
    Message = FString::Printf(TEXT("%s utilise %s (%d PM)."), *FindSpecies(Friends[ActiveFriend].SpeciesId)->Name,
        UTF8_TO_TCHAR(Entry.Name), Entry.MPCost);
    FinishAction();
    return true;
}

TArray<FFTInventoryItem> UFTAdventureSubsystem::GetInventory() const
{
    TArray<FFTInventoryItem> Result;
    for (const Item& Entry : ItemCatalog())
    {
        FFTInventoryItem View;
        View.Id = FName(UTF8_TO_TCHAR(Entry.Key));
        View.Name = UTF8_TO_TCHAR(Entry.Name); View.Description = UTF8_TO_TCHAR(Entry.Description);
        View.Quantity = Bag.Quantity(Entry.Id);
        View.bUsableOnActive = CanUseItem(View.Id, ActiveFriend);
        Result.Add(View);
    }
    return Result;
}

int32 UFTAdventureSubsystem::GetItemQuantity(FName Id) const
{
    const Item* Entry = FindItem(TCHAR_TO_UTF8(*Id.ToString()));
    return Entry ? Bag.Quantity(Entry->Id) : 0;
}

bool UFTAdventureSubsystem::CanUseItem(FName Id, int32 Index) const
{
    const Item* Entry = FindItem(TCHAR_TO_UTF8(*Id.ToString()));
    if (!Entry || !Dialogue.IsEmpty() || bAwaitingReplacement || !Friends.IsValidIndex(Index)) return false;
    if (bInBattle) return Index == ActiveFriend && Battle.CanUseItem(Bag, Entry->Id);
    const FFTMonsterStats Target = GetFriendStats(Index);
    return Bag.Quantity(Entry->Id) > 0 && CanRestore(*Entry, {Target.HP, Target.MP}, Target.MaxHP, Target.MaxMP);
}

bool UFTAdventureSubsystem::UseItem(FName Id, int32 Index)
{
    if (!CanUseItem(Id, Index))
    {
        Message = TEXT("Objet indisponible ou sans effet sur cette cible. Aucun objet ni tour dépensé.");
        return false;
    }
    const Item& Entry = *FindItem(TCHAR_TO_UTF8(*Id.ToString()));
    if (bInBattle)
    {
        if (!Battle.UseItem(Bag, Entry.Id)) return false;
        Message = Entry.Effect == ItemEffect::Befriend ?
            TEXT("La friandise rassure la créature : +20 points d'amitié jusqu'à la fin du combat. Elle riposte.") :
            FString::Printf(TEXT("%s utilisé sur %s. La créature riposte."), UTF8_TO_TCHAR(Entry.Name),
                *FindSpecies(Friends[Index].SpeciesId)->Name);
        FinishAction();
    }
    else
    {
        const FFTMonsterStats Stats = GetFriendStats(Index);
        Resources Target{Stats.HP, Stats.MP};
        if (!UseRestorative(Bag, Entry.Id, Target, Stats.MaxHP, Stats.MaxMP)) return false;
        Friends[Index].HP = Target.HP; Friends[Index].MP = Target.MP;
        Message = FString::Printf(TEXT("%s : %s récupère %d %s."), UTF8_TO_TCHAR(Entry.Name),
            *FindSpecies(Friends[Index].SpeciesId)->Name, Target.HP - Stats.HP + Target.MP - Stats.MP,
            Entry.Effect == ItemEffect::HealHP ? TEXT("PV") : TEXT("PM"));
        if (!SaveProgress()) Message += TEXT(" Sauvegarde impossible : réessayez avec Sauver.");
    }
    return true;
}

FString UFTAdventureSubsystem::AwardItems()
{
    const Inventory Loot = VictoryLoot(Kind(EnemyId));
    FString Summary;
    for (const Item& Entry : ItemCatalog())
    {
        const int32 Quantity = Loot.Quantity(Entry.Id);
        if (!Quantity) continue;
        const int32 Added = Bag.Add(Entry.Id, Quantity);
        Summary += Added > 0 ? FString::Printf(TEXT(" +%d %s."), Added, UTF8_TO_TCHAR(Entry.Name)) :
            FString::Printf(TEXT(" %s : pile pleine (99)."), UTF8_TO_TCHAR(Entry.Name));
    }
    return Summary;
}

void UFTAdventureSubsystem::Recruit()
{
    if (Friends.Num() >= 1000) { Message = TEXT("La collection de ce prototype est pleine."); return; }
    if (!bInBattle || bAwaitingReplacement || !Battle.TryRecruit(FMath::Min(FMath::FRand(), 0.99999994f))) return;
    Message = TEXT("La créature hésite encore... puis riposte. Essayez de mieux l'apprivoiser.");
    FinishAction();
}

void UFTAdventureSubsystem::Escape()
{
    if (!bInBattle || bAwaitingReplacement || !Battle.Escape()) return;
    Message = TEXT("Vous reculez doucement. Aucun gain d'XP ; les PV et PM dépensés restent consommés.");
    FinishAction();
}

FString UFTAdventureSubsystem::AwardExperience()
{
    FString Summary;
    const int32 Reward = EncounterReward(Kind(EnemyId), EnemyLevel);
    for (FFTMonsterFriend& Friend : Friends)
    {
        if (!BattleParticipants.Contains(Friend.InstanceId) || Friend.HP <= 0) continue;
        const int32 OldXP = Friend.Experience;
        const int32 OldLevel = LevelFromXP(OldXP);
        Friend.Experience = GrantXP(OldXP, Reward);
        if (Friend.Experience == OldXP) continue;
        const int32 Level = LevelFromXP(Friend.Experience);
        const FinalTowerRules::Species Id = Kind(Friend.SpeciesId);
        Summary += FString::Printf(TEXT(" %s +%d XP."), *FindSpecies(Friend.SpeciesId)->Name, Friend.Experience - OldXP);
        if (Level > OldLevel)
        {
            const Resources Growth = OnLevelGrowth({Friend.HP, Friend.MP}, StatsAtLevel(Id, OldLevel), StatsAtLevel(Id, Level));
            Friend.HP = Growth.HP; Friend.MP = Growth.MP;
            Summary += FString::Printf(TEXT(" Niveau %d !"), Level);
            for (SkillId Technique : SkillsForSpecies(Id))
                if (!HasLearned(Id, OldLevel, Technique) && HasLearned(Id, Level, Technique))
                    Summary += FString::Printf(TEXT(" Technique apprise : %s."), UTF8_TO_TCHAR(FindSkill(Technique)->Name));
        }
    }
    return Summary;
}

void UFTAdventureSubsystem::FinishAction()
{
    if (!bInBattle) return; // A completed encounter cannot grant its rewards a second time.
    if (Friends.IsValidIndex(ActiveFriend))
    {
        Friends[ActiveFriend].HP = Battle.Ally.HP;
        Friends[ActiveFriend].MP = Battle.Ally.MP;
    }
    using FinalTowerRules::Outcome;
    if (Battle.Result == Outcome::Ongoing) return;
    if (Battle.Result == Outcome::Defeat)
    {
        for (int32 Index : Party)
            if (Friends.IsValidIndex(Index) && Friends[Index].HP > 0)
            {
                bAwaitingReplacement = true;
                Message = TEXT("Votre compagnon est K.-O. Choisissez un autre membre de l'équipe : le remplacement est gratuit.");
                return;
            }
    }
    bInBattle = false; bAwaitingReplacement = false;
    const FString Rewards = (Battle.Result == Outcome::Victory || Battle.Result == Outcome::Recruited) ? AwardExperience() : FString();
    switch (Battle.Result)
    {
    case Outcome::Recruited:
        AddFriend(EnemyId, EnemyLevel, Battle.Enemy.HP);
        Message = EnemyId == TEXT("luciolle") ? TEXT("Luciolle se pose sur votre épaule. Une mélodie vient de sa lumière...") :
            FString::Printf(TEXT("%s rejoint vos compagnons."), *FindSpecies(EnemyId)->Name);
        Message += Rewards;
        break;
    case Outcome::Victory:
        Message = TEXT("La créature s'éloigne. Vous pourrez retenter l'amitié.") + Rewards + AwardItems();
        break;
    case Outcome::Defeat:
        HealFriends();
        Message = TEXT("Votre équipe se repose et récupère ses PV et PM. Aucun gain d'XP pour cette défaite.");
        break;
    default: break;
    }
    BattleParticipants.Reset();
    if (!SaveProgress()) Message += TEXT(" Sauvegarde impossible : réessayez avec Sauver.");
}

void UFTAdventureSubsystem::HealFriends()
{
    for (int32 Index = 0; Index < Friends.Num(); ++Index)
    {
        const FFTMonsterStats Stats = GetFriendStats(Index);
        Friends[Index].HP = Stats.MaxHP; Friends[Index].MP = Stats.MaxMP;
    }
}

bool UFTAdventureSubsystem::IsDiscoveryCollected(FName Id) const
{
    const Discovery* Entry = FindDiscovery(TCHAR_TO_UTF8(*Id.ToString()));
    return Entry && WorldProgress.IsCollected(Entry->Id);
}

int32 UFTAdventureSubsystem::GetDiscoveryCount() const { return WorldProgress.DiscoveredCount(); }
int32 UFTAdventureSubsystem::GetWoundedStage() const { return static_cast<int32>(WorldProgress.Wounded); }
bool UFTAdventureSubsystem::IsWoundedFollowing() const { return WorldProgress.ReadyToReturn(); }

bool UFTAdventureSubsystem::CollectDiscovery(FName Id)
{
    if (bInBattle || !Dialogue.IsEmpty()) return false;
    const Discovery* Entry = FindDiscovery(TCHAR_TO_UTF8(*Id.ToString()));
    if (!Entry) return false;
    const CollectResult Result = FinalTowerRules::CollectDiscovery(WorldProgress, Bag, Entry->Id);
    if (Result == CollectResult::AlreadyCollected) { Message = TEXT("Cette découverte a déjà été récupérée."); return false; }
    if (Result == CollectResult::BagFull)
    {
        Message = TEXT("Une pile du sac est trop pleine. Utilisez des objets puis revenez : tout reste sur place.");
        return false;
    }
    if (Result != CollectResult::Collected) return false;
    if (Entry->Kind == DiscoveryKind::Clue)
        Message = TEXT("Dans le nid : un ruban et un petit bateau gravé « Lina ». L'indice est inscrit au journal.");
    else
    {
        Message = FString::Printf(TEXT("%s :"), UTF8_TO_TCHAR(Entry->Name));
        const Inventory Loot = DiscoveryLoot(*Entry);
        for (const Item& Object : ItemCatalog())
            if (Loot.Quantity(Object.Id) > 0)
                Message += FString::Printf(TEXT(" +%d %s."), Loot.Quantity(Object.Id), UTF8_TO_TCHAR(Object.Name));
    }
    if (WorldProgress.ReadyToReturn()) Message += TEXT(" Mousse vous suit. Retournez voir la gardienne.");
    if (!SaveProgress()) Message += TEXT(" Sauvegarde impossible : réessayez avec Sauver.");
    return true;
}

void UFTAdventureSubsystem::TalkToWounded()
{
    if (bInBattle || !Dialogue.IsEmpty() || WorldProgress.Wounded == WoundedStep::Completed) return;
    const WoundedStep Before = WorldProgress.Wounded;
    if (Before == WoundedStep::Found && Bag.Quantity(ItemId::Berry) == 0)
    {
        Message = TEXT("Il faut une baie de braise. Cherchez le buisson plus au nord ou le coffre à l'est de la maison.");
        return;
    }
    if (Before == WoundedStep::Healed && Bag.Quantity(ItemId::Treat) == 0)
    {
        Message = TEXT("Il faut une friandise des bois. Fouillez les coffres ou gagnez un combat contre Roncinet.");
        return;
    }
    FinalTowerRules::HelpWounded(WorldProgress, Bag);
    DialogueLines.Reset();
    bKeeperDialogue = bFinishStoryAfterDialogue = bFinishWoundedAfterDialogue = false;
    if (Before == WoundedStep::Hidden)
    {
        DialogueLines.Add(TEXT("Une petite Mousse tremble sous un arbre. Sa patte est blessée ; un fil de ruban pend à sa feuille."));
        DialogueLines.Add(TEXT("Elle recule lorsque vous regardez vers le village. Une baie de braise pourrait soigner sa patte."));
    }
    else if (Before == WoundedStep::Found)
    {
        DialogueLines.Add(TEXT("Vous écrasez une baie de braise et l'appliquez sur la patte. Mousse cesse de trembler. Une baie a été utilisée."));
        DialogueLines.Add(TEXT("Elle pourrait marcher, mais refuse toujours de partir. Peut-être qu'une friandise l'aiderait à vous faire confiance."));
    }
    else if (Before == WoundedStep::Healed)
    {
        DialogueLines.Add(TEXT("Vous tendez une friandise des bois. Mousse vient manger au creux de votre gant. Une friandise a été utilisée."));
        DialogueLines.Add(WorldProgress.ReadyToReturn() ?
            TEXT("Elle reconnaît le ruban du nid que vous avez trouvé. Cette fois, elle accepte de vous suivre jusqu'à la gardienne.") :
            TEXT("Elle tire doucement votre manche vers le nord-ouest, puis revient sous son arbre. Quelque chose l'attend dans un nid sous les branches."));
    }
    else
    {
        DialogueLines.Add(WorldProgress.ReadyToReturn() ?
            TEXT("Mousse vous suit sans crainte. Le nom gravé sur le bateau dira peut-être quelque chose à la gardienne.") :
            TEXT("Mousse regarde vers le nid, au nord-ouest. Cherchez ce qu'elle protège sous les branches."));
    }
    DialogueIndex = 0;
    Dialogue = DialogueLines[0];
}

TArray<FFTQuestEntry> UFTAdventureSubsystem::GetQuests() const
{
    TArray<FFTQuestEntry> Result;
    FFTQuestEntry Lantern;
    Lantern.Id = TEXT("keepers_lantern"); Lantern.Title = TEXT("La lanterne du passeur");
    Lantern.bComplete = bStoryComplete;
    Lantern.Status = bStoryComplete ? TEXT("Terminée") : (bHeardKeeper || HasFriend(TEXT("luciolle")) ? TEXT("En cours") : TEXT("À découvrir"));
    Lantern.Objective = GetLanternObjective();
    Lantern.Clues = HasFriend(TEXT("luciolle")) ? TEXT("Luciolle fredonne la mélodie de la fille de la gardienne.") :
        (bHeardKeeper ? TEXT("Une lumière chantait près des roseaux, au nord-est du village.") : TEXT("Une gardienne veille près d'une lanterne éteinte."));
    Lantern.Reward = TEXT("Rendre sa lumière à la lanterne et découvrir le souvenir de la gardienne.");
    Result.Add(Lantern);
    FFTQuestEntry Wounded;
    Wounded.Id = TEXT("wounded_mousse"); Wounded.Title = TEXT("Celui qui attend sous les branches");
    Wounded.bComplete = WorldProgress.Wounded == WoundedStep::Completed;
    Wounded.Status = Wounded.bComplete ? TEXT("Terminée") : (WorldProgress.Wounded == WoundedStep::Hidden ? TEXT("À découvrir") :
        (WorldProgress.ReadyToReturn() ? TEXT("Retour à la gardienne") : TEXT("En cours")));
    Wounded.Objective = UTF8_TO_TCHAR(WoundedObjective(WorldProgress));
    Wounded.Clues = WorldProgress.IsCollected(DiscoveryId::OldNest) ?
        TEXT("Dans le nid, un ruban usé et un bateau de bois portent le nom « Lina ». Mousse les gardait à l'abri.") :
        (WorldProgress.Wounded == WoundedStep::Hidden ? TEXT("Des feuilles bougent à l'ouest du village.") :
            TEXT("Une patte blessée, un fil de ruban et une créature qui refuse d'abandonner les arbres."));
    Wounded.Reward = TEXT("Mousse niveau 2 rejoint la collection ; 2 rosées de lune. Récompense unique.");
    Result.Add(Wounded);
    return Result;
}

void UFTAdventureSubsystem::TalkToKeeper()
{
    if (bInBattle || !Dialogue.IsEmpty()) return;
    HealFriends();
    DialogueLines.Reset();
    bFinishStoryAfterDialogue = bFinishWoundedAfterDialogue = false;
    bKeeperDialogue = true;
    if (WorldProgress.ReadyToReturn())
    {
        if (Friends.Num() >= 1000 || !FitsLoot(Bag, WoundedReward()))
        {
            Message = TEXT("Mousse reste avec vous. Pour recevoir sa récompense, gardez deux places dans la pile de rosées et une place dans la collection.");
            return;
        }
        bKeeperDialogue = false;
        bFinishWoundedAfterDialogue = true;
        DialogueLines.Add(TEXT("Gardienne : Ce bateau... Lina l'avait fabriqué pour une petite Mousse. Elle lui promettait de revenir chaque printemps."));
        DialogueLines.Add(TEXT("Gardienne : Tu gardais encore ses affaires au sec ? Oh, petite... tu n'as plus besoin d'attendre toute seule."));
        DialogueLines.Add(TEXT("La gardienne attache le ruban à votre gant. Mousse s'y blottit. Certains souvenirs peuvent voyager avec nous."));
        DialogueIndex = 0; Dialogue = DialogueLines[0];
        return;
    }
    if (bStoryComplete)
    {
        DialogueLines.Add(TEXT("Gardienne : La lanterne brille à nouveau. Prenez soin de votre petite amie."));
    }
    else if (HasFriend(TEXT("luciolle")))
    {
        DialogueLines.Add(TEXT("Gardienne : Cette mélodie... ma fille la chantait en rentrant du fleuve."));
        DialogueLines.Add(TEXT("Gardienne : Je croyais avoir oublié sa voix. Merci de m'en avoir rendu un petit morceau."));
        DialogueLines.Add(TEXT("Luciolle effleure la lanterne. Pour la première fois depuis des années, elle s'allume."));
        bFinishStoryAfterDialogue = true;
    }
    else if (!bHeardKeeper)
    {
        DialogueLines.Add(TEXT("Gardienne : Chaque soir, j'allumais cette lanterne pour guider ma fille jusqu'à la maison."));
        DialogueLines.Add(TEXT("Gardienne : Elle est partie il y a longtemps. Hier, une lumière chantait son air près des roseaux."));
        DialogueLines.Add(TEXT("Gardienne : Si vous la rencontrez, ne la chassez pas. Peut-être acceptera-t-elle de vous suivre."));
    }
    else
    {
        DialogueLines.Add(TEXT("Gardienne : Cherchez près des roseaux. Votre compagnon est reposé ; allez-y doucement."));
    }
    DialogueIndex = 0;
    Dialogue = DialogueLines[0];
}

void UFTAdventureSubsystem::AdvanceDialogue()
{
    if (Dialogue.IsEmpty()) return;
    ++DialogueIndex;
    if (DialogueLines.IsValidIndex(DialogueIndex))
    {
        Dialogue = DialogueLines[DialogueIndex];
        return;
    }
    Dialogue.Empty();
    if (bKeeperDialogue)
    {
        bHeardKeeper = true;
        if (bFinishStoryAfterDialogue) bStoryComplete = true;
    }
    if (bFinishWoundedAfterDialogue)
    {
        if (ClaimWoundedReward(WorldProgress, Bag, Friends.Num()))
        {
            AddFriend(TEXT("mousse"), 2);
            Message = TEXT("Histoire terminée : Celui qui attend sous les branches. Mousse niv. 2 vous rejoint ; +2 rosées de lune.");
        }
        else Message = TEXT("La récompense attend chez la gardienne. Libérez de la place et reparlez-lui.");
    }
    else Message = GetObjective();
    bFinishStoryAfterDialogue = bFinishWoundedAfterDialogue = bKeeperDialogue = false;
    if (!SaveProgress()) Message += TEXT(" Sauvegarde impossible : réessayez avec Sauver.");
}

FString UFTAdventureSubsystem::GetObjective() const
{
    if (WorldProgress.Wounded != WoundedStep::Hidden && WorldProgress.Wounded != WoundedStep::Completed)
        return UTF8_TO_TCHAR(WoundedObjective(WorldProgress));
    if (bStoryComplete && WorldProgress.Wounded == WoundedStep::Completed)
        return FString::Printf(TEXT("Explorer les environs — %d/%d découvertes"), GetDiscoveryCount(), DiscoveryCount);
    return GetLanternObjective();
}

FString UFTAdventureSubsystem::GetLanternObjective() const
{
    if (bStoryComplete) return TEXT("La lanterne du passeur — histoire terminée");
    if (HasFriend(TEXT("luciolle"))) return TEXT("Retourner voir la gardienne avec Luciolle");
    if (bHeardKeeper) return TEXT("Se lier d'amitié avec la lumière des roseaux");
    return TEXT("Parler à la gardienne près de la lanterne");
}

void UFTAdventureSubsystem::NextCompanion()
{
    if (bInBattle || !Dialogue.IsEmpty() || Party.Num() < 2) return;
    const int32 Position = Party.Find(ActiveFriend);
    for (int32 Offset = 1; Offset < Party.Num(); ++Offset)
    {
        const int32 Index = Party[(FMath::Max(0, Position) + Offset) % Party.Num()];
        if (Friends[Index].HP > 0) { SetActiveFriend(Index); return; }
    }
}

bool UFTAdventureSubsystem::SetActiveFriend(int32 Index)
{
    if (!Friends.IsValidIndex(Index) || !Party.Contains(Index) || Friends[Index].HP <= 0 ||
        Index == ActiveFriend || !Dialogue.IsEmpty()) return false;
    if (bInBattle)
    {
        const FFTMonsterStats Replacement = GetFriendStats(Index);
        if (!Battle.SwitchAlly(MakeFighter(Replacement), Kind(Friends[Index].SpeciesId), Replacement.Level)) return false;
        ActiveFriend = Index;
        BattleParticipants.Add(Friends[Index].InstanceId);
        bAwaitingReplacement = false;
        Message = FString::Printf(TEXT("%s entre en combat."), *FindSpecies(Friends[Index].SpeciesId)->Name);
        FinishAction();
    }
    else
    {
        ActiveFriend = Index;
        Message = FString::Printf(TEXT("%s prend la tête de l'équipe."), *FindSpecies(Friends[Index].SpeciesId)->Name);
        if (!SaveProgress()) Message += TEXT(" Sauvegarde impossible.");
    }
    return true;
}

bool UFTAdventureSubsystem::TogglePartyMember(int32 Index)
{
    if (bInBattle || !Dialogue.IsEmpty() || !Friends.IsValidIndex(Index)) return false;
    if (Party.Contains(Index))
    {
        if (Party.Num() == 1) { Message = TEXT("L'équipe doit conserver au moins un compagnon."); return false; }
        if (Index == ActiveFriend)
        {
            int32 Next = INDEX_NONE;
            for (int32 Other : Party) if (Other != Index && Friends[Other].HP > 0) { Next = Other; break; }
            if (Next == INDEX_NONE) { Message = TEXT("Il faut un compagnon en état de combattre à la tête de l'équipe."); return false; }
            ActiveFriend = Next;
        }
        Party.Remove(Index);
        Message = TEXT("Compagnon placé en réserve.");
    }
    else
    {
        if (Party.Num() >= PartyCapacity) { Message = TEXT("Équipe complète : placez un membre en réserve avant d'en ajouter un autre."); return false; }
        Party.Add(Index);
        Message = TEXT("Compagnon ajouté à l'équipe.");
    }
    if (!SaveProgress()) Message += TEXT(" Sauvegarde impossible.");
    return true;
}

bool UFTAdventureSubsystem::SaveProgress()
{
    if (bInBattle || !Dialogue.IsEmpty()) return false;
    UFTAdventureSave* Save = Cast<UFTAdventureSave>(UGameplayStatics::CreateSaveGameObject(UFTAdventureSave::StaticClass()));
    if (!Save) return false;
    Save->Version = 4;
    Save->WoundedStage = static_cast<int32>(WorldProgress.Wounded);
    for (const Discovery& Entry : DiscoveryCatalog())
        if (WorldProgress.IsCollected(Entry.Id)) Save->CollectedDiscoveries.Add(FName(UTF8_TO_TCHAR(Entry.Key)));
    for (const Item& Entry : ItemCatalog())
        Save->Items.Add(FName(UTF8_TO_TCHAR(Entry.Key)), Bag.Quantity(Entry.Id));
    Save->Friends = Friends; Save->Party = Party;
    Save->ActiveFriend = ActiveFriend;
    Save->bHeardKeeper = bHeardKeeper; Save->bStoryComplete = bStoryComplete;
    return UGameplayStatics::SaveGameToSlot(Save, SaveSlot, 0);
}

bool UFTAdventureSubsystem::LoadProgress()
{
    if (bInBattle || !Dialogue.IsEmpty() || !UGameplayStatics::DoesSaveGameExist(SaveSlot, 0)) return false;
    const UFTAdventureSave* Save = Cast<UFTAdventureSave>(UGameplayStatics::LoadGameFromSlot(SaveSlot, 0));
    if (!Save || (Save->Version < 1 || Save->Version > 4) || Save->Friends.IsEmpty() || Save->Friends.Num() > 1000) return false;
    TSet<FGuid> Seen;
    for (const FFTMonsterFriend& Friend : Save->Friends)
    {
        if (!FindSpecies(Friend.SpeciesId) || !Friend.InstanceId.IsValid() || Seen.Contains(Friend.InstanceId)) return false;
        Seen.Add(Friend.InstanceId);
    }
    TArray<FFTMonsterFriend> Validated = Save->Friends;
    for (FFTMonsterFriend& Friend : Validated)
    {
        Friend.Experience = Save->Version == 1 ? 0 : GrantXP(Friend.Experience, 0);
        const MonsterStats Stats = StatsAtLevel(Kind(Friend.SpeciesId), LevelFromXP(Friend.Experience));
        Friend.HP = Clamp(Friend.HP, 0, Stats.MaxHP);
        Friend.MP = Save->Version == 1 ? Stats.MaxMP : Clamp(Friend.MP, 0, Stats.MaxMP);
    }
    std::vector<int> Input;
    if (Save->Version >= 2) for (int32 Index : Save->Party) Input.push_back(Index);
    const auto Normalized = NormalizeParty(Input, Validated.Num(), Save->ActiveFriend);
    Inventory SavedBag;
    for (const Item& Entry : ItemCatalog())
    {
        const int32* Count = Save->Items.Find(FName(UTF8_TO_TCHAR(Entry.Key)));
        if (Count) SavedBag.Add(Entry.Id, *Count);
    }
    Exploration SavedWorld;
    if (Save->Version >= 4)
    {
        SavedWorld.Wounded = static_cast<WoundedStep>(Save->WoundedStage);
        for (const Discovery& Entry : DiscoveryCatalog())
            SavedWorld.Collected[static_cast<int>(Entry.Id)] = Save->CollectedDiscoveries.Contains(FName(UTF8_TO_TCHAR(Entry.Key)));
    }
    WorldProgress = RestoreExploration(Save->Version, SavedWorld);
    Bag = RestoreInventory(Save->Version, SavedBag);
    Friends = MoveTemp(Validated);
    Party.Reset(); for (int32 Index : Normalized) Party.Add(Index);
    ActiveFriend = Party.Contains(Save->ActiveFriend) ? Save->ActiveFriend : Party[0];
    bool Alive = false;
    for (int32 Index : Party) if (Friends[Index].HP > 0) Alive = true;
    if (!Alive) HealFriends();
    if (Friends[ActiveFriend].HP == 0)
        for (int32 Index : Party) if (Friends[Index].HP > 0) { ActiveFriend = Index; break; }
    bHeardKeeper = Save->bHeardKeeper;
    bStoryComplete = Save->bStoryComplete && HasFriend(TEXT("luciolle"));
    Message = Save->Version == 1 ? TEXT("Ancienne sauvegarde retrouvée : équipe et PM initialisés, compagnons conservés.") :
        TEXT("Aventure retrouvée : compagnons, sac, découvertes et journal restaurés.");
    if (Save->Version < 3) Message += TEXT(" Sac de départ reçu : 3 baies, 2 rosées et 2 friandises.");
    return true;
}
