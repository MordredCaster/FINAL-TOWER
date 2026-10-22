#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameFramework/SaveGame.h"
#include "FTBattleRules.h"
#include "FTExplorationRules.h"
#include "FTAdventureSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FINALTOWER_API FFTMonsterSpecies
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Id;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Name;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MaxHP = 30;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Attack = 6;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Defense = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MaxMP = 10;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Magic = 7;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Role;
};

USTRUCT(BlueprintType)
struct FINALTOWER_API FFTMonsterFriend
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, BlueprintReadOnly) FGuid InstanceId;
    UPROPERTY(SaveGame, BlueprintReadOnly) FName SpeciesId;
    UPROPERTY(SaveGame, BlueprintReadOnly) int32 HP = 1;
    UPROPERTY(SaveGame, BlueprintReadOnly) int32 MP = -1;
    UPROPERTY(SaveGame, BlueprintReadOnly) int32 Experience = 0;
};

USTRUCT(BlueprintType)
struct FINALTOWER_API FFTMonsterStats
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) int32 Level = 1;
    UPROPERTY(BlueprintReadOnly) int32 HP = 0;
    UPROPERTY(BlueprintReadOnly) int32 MaxHP = 1;
    UPROPERTY(BlueprintReadOnly) int32 MP = 0;
    UPROPERTY(BlueprintReadOnly) int32 MaxMP = 0;
    UPROPERTY(BlueprintReadOnly) int32 Attack = 1;
    UPROPERTY(BlueprintReadOnly) int32 Defense = 0;
    UPROPERTY(BlueprintReadOnly) int32 Magic = 0;
    UPROPERTY(BlueprintReadOnly) int32 Experience = 0;
    UPROPERTY(BlueprintReadOnly) int32 NextLevelXP = 0;
};

USTRUCT(BlueprintType)
struct FINALTOWER_API FFTMonsterTechnique
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FString Name;
    UPROPERTY(BlueprintReadOnly) FString Description;
    UPROPERTY(BlueprintReadOnly) int32 UnlockLevel = 1;
    UPROPERTY(BlueprintReadOnly) int32 MPCost = 0;
    UPROPERTY(BlueprintReadOnly) bool bLearned = false;
    UPROPERTY(BlueprintReadOnly) bool bUsable = false;
};

USTRUCT(BlueprintType)
struct FINALTOWER_API FFTInventoryItem
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FName Id;
    UPROPERTY(BlueprintReadOnly) FString Name;
    UPROPERTY(BlueprintReadOnly) FString Description;
    UPROPERTY(BlueprintReadOnly) int32 Quantity = 0;
    UPROPERTY(BlueprintReadOnly) bool bUsableOnActive = false;
};

USTRUCT(BlueprintType)
struct FINALTOWER_API FFTQuestEntry
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FName Id;
    UPROPERTY(BlueprintReadOnly) FString Title;
    UPROPERTY(BlueprintReadOnly) FString Status;
    UPROPERTY(BlueprintReadOnly) FString Objective;
    UPROPERTY(BlueprintReadOnly) FString Clues;
    UPROPERTY(BlueprintReadOnly) FString Reward;
    UPROPERTY(BlueprintReadOnly) bool bComplete = false;
};

UCLASS()
class FINALTOWER_API UFTAdventureSave : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame) int32 Version = 4;
    UPROPERTY(SaveGame) TArray<FFTMonsterFriend> Friends;
    UPROPERTY(SaveGame) bool bHeardKeeper = false;
    UPROPERTY(SaveGame) bool bStoryComplete = false;
    UPROPERTY(SaveGame) int32 ActiveFriend = 0;
    UPROPERTY(SaveGame) TArray<int32> Party;
    UPROPERTY(SaveGame) TMap<FName, int32> Items;
    UPROPERTY(SaveGame) TArray<FName> CollectedDiscoveries;
    UPROPERTY(SaveGame) int32 WoundedStage = 0;
};

/** Session state shared by the native prototype and future Blueprint/UI assets. */
UCLASS()
class FINALTOWER_API UFTAdventureSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UPROPERTY(BlueprintReadOnly) TArray<FFTMonsterSpecies> Species;
    UPROPERTY(BlueprintReadOnly) TArray<FFTMonsterFriend> Friends;
    UPROPERTY(BlueprintReadOnly) int32 ActiveFriend = 0;
    UPROPERTY(BlueprintReadOnly) bool bInBattle = false;
    UPROPERTY(BlueprintReadOnly) bool bAwaitingReplacement = false;
    UPROPERTY(BlueprintReadOnly) TArray<int32> Party;
    UPROPERTY(BlueprintReadOnly) bool bHeardKeeper = false;
    UPROPERTY(BlueprintReadOnly) bool bStoryComplete = false;
    UPROPERTY(BlueprintReadOnly) FString Message;
    UPROPERTY(BlueprintReadOnly) FString Dialogue;

    UFUNCTION(BlueprintCallable) bool BeginEncounter(FName SpeciesId);
    UFUNCTION(BlueprintCallable) void Attack();
    UFUNCTION(BlueprintCallable) void Recruit();
    UFUNCTION(BlueprintCallable) void Escape();
    UFUNCTION(BlueprintCallable) void TalkToKeeper();
    UFUNCTION(BlueprintCallable) void AdvanceDialogue();
    UFUNCTION(BlueprintCallable) bool SaveProgress();
    UFUNCTION(BlueprintCallable) bool LoadProgress();
    UFUNCTION(BlueprintCallable) void NextCompanion();
    UFUNCTION(BlueprintCallable) bool UseSkill(int32 SkillIndex);
    UFUNCTION(BlueprintCallable) bool SetActiveFriend(int32 RosterIndex);
    UFUNCTION(BlueprintCallable) bool TogglePartyMember(int32 RosterIndex);
    UFUNCTION(BlueprintPure) FFTMonsterStats GetFriendStats(int32 RosterIndex) const;
    UFUNCTION(BlueprintPure) TArray<FFTMonsterTechnique> GetTechniques(int32 RosterIndex) const;
    UFUNCTION(BlueprintPure) bool HasFriend(FName SpeciesId) const;
    UFUNCTION(BlueprintPure) FString GetObjective() const;
    UFUNCTION(BlueprintCallable) bool UseItem(FName ItemId, int32 TargetIndex);
    UFUNCTION(BlueprintPure) bool CanUseItem(FName ItemId, int32 TargetIndex) const;
    UFUNCTION(BlueprintPure) int32 GetItemQuantity(FName ItemId) const;
    UFUNCTION(BlueprintPure) TArray<FFTInventoryItem> GetInventory() const;
    UFUNCTION(BlueprintCallable) bool CollectDiscovery(FName DiscoveryId);
    UFUNCTION(BlueprintCallable) void TalkToWounded();
    UFUNCTION(BlueprintPure) bool IsDiscoveryCollected(FName DiscoveryId) const;
    UFUNCTION(BlueprintPure) int32 GetDiscoveryCount() const;
    UFUNCTION(BlueprintPure) int32 GetWoundedStage() const;
    UFUNCTION(BlueprintPure) bool IsWoundedFollowing() const;
    UFUNCTION(BlueprintPure) TArray<FFTQuestEntry> GetQuests() const;

    const FFTMonsterSpecies* FindSpecies(FName Id) const;
    const FinalTowerRules::Battle& GetBattle() const { return Battle; }
    const FFTMonsterSpecies* GetEnemySpecies() const { return FindSpecies(EnemyId); }
    int32 GetEnemyLevel() const { return EnemyLevel; }
    FName GetEncounterId() const { return EnemyId; }

private:
    FinalTowerRules::Battle Battle;
    FName EnemyId;
    int32 EnemyLevel = 1;
    TSet<FGuid> BattleParticipants;
    FinalTowerRules::Inventory Bag;
    FinalTowerRules::Exploration WorldProgress;
    TArray<FString> DialogueLines;
    int32 DialogueIndex = 0;
    bool bFinishStoryAfterDialogue = false;
    bool bKeeperDialogue = false;
    bool bFinishWoundedAfterDialogue = false;
    void AddFriend(FName SpeciesId, int32 Level = 1, int32 HP = -1);
    void FinishAction();
    void HealFriends();
    FString AwardExperience();
    FString AwardItems();
    FString GetLanternObjective() const;
};
