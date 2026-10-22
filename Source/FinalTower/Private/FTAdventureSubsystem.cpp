#include "FTAdventureSubsystem.h"
#include "Kismet/GameplayStatics.h"

namespace
{
const FString SaveSlot = TEXT("FinalTower_Prototype_v1");

FFTMonsterSpecies MakeSpecies(FName Id, const TCHAR* Name, int32 HP, int32 Attack, int32 Defense)
{
    FFTMonsterSpecies Result;
    Result.Id = Id;
    Result.Name = Name;
    Result.MaxHP = HP;
    Result.Attack = Attack;
    Result.Defense = Defense;
    return Result;
}

FinalTowerRules::Fighter MakeFighter(const FFTMonsterSpecies& Species, int32 HP)
{
    return { Species.MaxHP, HP, Species.Attack, Species.Defense };
}
}

void UFTAdventureSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Species = {
        MakeSpecies(TEXT("mousse"), TEXT("Mousse"), 40, 8, 2),
        MakeSpecies(TEXT("luciolle"), TEXT("Luciolle"), 30, 6, 1)
    };
    if (!LoadProgress())
    {
        AddFriend(TEXT("mousse"));
        Message = TEXT("Mousse vous accompagne. Une gardienne attend près de la lanterne.");
    }
}

const FFTMonsterSpecies* UFTAdventureSubsystem::FindSpecies(FName Id) const
{
    return Species.FindByPredicate([Id](const FFTMonsterSpecies& Entry) { return Entry.Id == Id; });
}

bool UFTAdventureSubsystem::HasFriend(FName Id) const
{
    return Friends.ContainsByPredicate([Id](const FFTMonsterFriend& Entry) { return Entry.SpeciesId == Id; });
}

void UFTAdventureSubsystem::AddFriend(FName Id)
{
    if (const FFTMonsterSpecies* Entry = FindSpecies(Id))
    {
        FFTMonsterFriend Friend;
        Friend.InstanceId = FGuid::NewGuid();
        Friend.SpeciesId = Id;
        Friend.HP = Entry->MaxHP;
        Friends.Add(Friend);
    }
}

bool UFTAdventureSubsystem::BeginEncounter(FName Id)
{
    if (bInBattle || !Dialogue.IsEmpty() || !Friends.IsValidIndex(ActiveFriend)) return false;
    const FFTMonsterSpecies* Enemy = FindSpecies(Id);
    const FFTMonsterSpecies* Ally = FindSpecies(Friends[ActiveFriend].SpeciesId);
    if (!Enemy || !Ally || Friends[ActiveFriend].HP <= 0) return false;
    EnemyId = Id;
    Battle = {};
    Battle.Ally = MakeFighter(*Ally, Friends[ActiveFriend].HP);
    Battle.Enemy = MakeFighter(*Enemy, Enemy->MaxHP);
    bInBattle = true;
    Message = FString::Printf(TEXT("%s vous observe. Affaiblissez-la, puis tendez la main."), *Enemy->Name);
    return true;
}

void UFTAdventureSubsystem::Attack()
{
    if (!bInBattle || !Battle.Attack()) return;
    Message = TEXT("Votre compagnon attaque. La créature riposte si elle tient encore debout.");
    FinishAction();
}

void UFTAdventureSubsystem::Recruit()
{
    if (!bInBattle) return;
    // Keep the sample strictly below 1 even on a platform whose RNG includes its upper endpoint.
    if (!Battle.TryRecruit(FMath::Min(FMath::FRand(), 0.99999994f))) return;
    Message = TEXT("Elle hésite encore... puis riposte. Essayez de mieux l'apprivoiser.");
    FinishAction();
}

void UFTAdventureSubsystem::Escape()
{
    if (!bInBattle || !Battle.Escape()) return;
    Message = TEXT("Vous reculez doucement. La rencontre pourra être retentée.");
    FinishAction();
}

void UFTAdventureSubsystem::FinishAction()
{
    if (Friends.IsValidIndex(ActiveFriend)) Friends[ActiveFriend].HP = Battle.Ally.HP;
    using FinalTowerRules::Outcome;
    if (Battle.Result == Outcome::Ongoing) return;
    bInBattle = false;
    switch (Battle.Result)
    {
    case Outcome::Recruited:
        AddFriend(EnemyId);
        Message = TEXT("Luciolle se pose sur votre épaule. Une mélodie semble venir de sa lumière...");
        break;
    case Outcome::Victory:
        Message = TEXT("La créature s'enfuit dans les roseaux. Elle reviendra : vous pourrez retenter l'amitié.");
        break;
    case Outcome::Defeat:
        HealFriends();
        Message = TEXT("Mousse se relève après une pause. La gardienne vous invite à réessayer.");
        break;
    default:
        break;
    }
    if (!SaveProgress()) Message += TEXT(" Sauvegarde impossible : réessayez avec Sauver.");
}

void UFTAdventureSubsystem::HealFriends()
{
    for (FFTMonsterFriend& Friend : Friends)
        if (const FFTMonsterSpecies* Entry = FindSpecies(Friend.SpeciesId)) Friend.HP = Entry->MaxHP;
}

void UFTAdventureSubsystem::TalkToKeeper()
{
    if (bInBattle || !Dialogue.IsEmpty()) return;
    HealFriends();
    DialogueLines.Reset();
    bFinishStoryAfterDialogue = false;
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
    bHeardKeeper = true;
    if (bFinishStoryAfterDialogue) bStoryComplete = true;
    bFinishStoryAfterDialogue = false;
    Message = bStoryComplete ? TEXT("Histoire terminée : La lanterne du passeur. Votre aventure ne fait que commencer.")
                            : TEXT("Cherchez la lumière près des roseaux et tendez-lui la main.");
    if (!SaveProgress()) Message += TEXT(" Sauvegarde impossible : réessayez avec Sauver.");
}

FString UFTAdventureSubsystem::GetObjective() const
{
    if (bStoryComplete) return TEXT("La lanterne du passeur — histoire terminée");
    if (HasFriend(TEXT("luciolle"))) return TEXT("Retourner voir la gardienne avec Luciolle");
    if (bHeardKeeper) return TEXT("Se lier d'amitié avec la lumière des roseaux");
    return TEXT("Parler à la gardienne près de la lanterne");
}

void UFTAdventureSubsystem::NextCompanion()
{
    if (bInBattle || !Dialogue.IsEmpty() || Friends.Num() < 2) return;
    ActiveFriend = (ActiveFriend + 1) % Friends.Num();
    const FFTMonsterSpecies* Entry = FindSpecies(Friends[ActiveFriend].SpeciesId);
    Message = Entry ? FString::Printf(TEXT("%s prend la tête de l'équipe."), *Entry->Name) : FString();
}

bool UFTAdventureSubsystem::SaveProgress()
{
    // Do not persist a partial dialogue or an unresolved fight.
    if (bInBattle || !Dialogue.IsEmpty()) return false;
    UFTAdventureSave* Save = Cast<UFTAdventureSave>(UGameplayStatics::CreateSaveGameObject(UFTAdventureSave::StaticClass()));
    if (!Save) return false;
    Save->Friends = Friends;
    Save->ActiveFriend = ActiveFriend;
    Save->bHeardKeeper = bHeardKeeper;
    Save->bStoryComplete = bStoryComplete;
    return UGameplayStatics::SaveGameToSlot(Save, SaveSlot, 0);
}

bool UFTAdventureSubsystem::LoadProgress()
{
    if (bInBattle || !Dialogue.IsEmpty()) return false;
    if (!UGameplayStatics::DoesSaveGameExist(SaveSlot, 0)) return false;
    const UFTAdventureSave* Save = Cast<UFTAdventureSave>(UGameplayStatics::LoadGameFromSlot(SaveSlot, 0));
    if (!Save || Save->Version != 1 || Save->Friends.IsEmpty() || Save->Friends.Num() > 1000) return false;
    // Validate before replacing the current session; unknown species must not become a broken party.
    TSet<FGuid> Seen;
    for (const FFTMonsterFriend& Friend : Save->Friends)
    {
        if (!FindSpecies(Friend.SpeciesId) || !Friend.InstanceId.IsValid() || Seen.Contains(Friend.InstanceId)) return false;
        Seen.Add(Friend.InstanceId);
    }
    Friends = Save->Friends;
    for (FFTMonsterFriend& Friend : Friends)
        Friend.HP = FMath::Clamp(Friend.HP, 0, FindSpecies(Friend.SpeciesId)->MaxHP);
    ActiveFriend = FMath::Clamp(Save->ActiveFriend, 0, Friends.Num() - 1);
    if (!Friends.ContainsByPredicate([](const FFTMonsterFriend& Friend) { return Friend.HP > 0; })) HealFriends();
    if (Friends[ActiveFriend].HP == 0)
        ActiveFriend = Friends.IndexOfByPredicate([](const FFTMonsterFriend& Friend) { return Friend.HP > 0; });
    bHeardKeeper = Save->bHeardKeeper;
    bStoryComplete = Save->bStoryComplete && HasFriend(TEXT("luciolle"));
    Message = TEXT("Aventure retrouvée. Vos compagnons vous attendent.");
    return true;
}
