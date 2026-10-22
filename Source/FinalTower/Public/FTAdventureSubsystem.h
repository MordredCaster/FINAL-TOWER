#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameFramework/SaveGame.h"
#include "FTBattleRules.h"
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
};

USTRUCT(BlueprintType)
struct FINALTOWER_API FFTMonsterFriend
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, BlueprintReadOnly) FGuid InstanceId;
    UPROPERTY(SaveGame, BlueprintReadOnly) FName SpeciesId;
    UPROPERTY(SaveGame, BlueprintReadOnly) int32 HP = 1;
};

UCLASS()
class FINALTOWER_API UFTAdventureSave : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame) int32 Version = 1;
    UPROPERTY(SaveGame) TArray<FFTMonsterFriend> Friends;
    UPROPERTY(SaveGame) bool bHeardKeeper = false;
    UPROPERTY(SaveGame) bool bStoryComplete = false;
    UPROPERTY(SaveGame) int32 ActiveFriend = 0;
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
    UFUNCTION(BlueprintPure) bool HasFriend(FName SpeciesId) const;
    UFUNCTION(BlueprintPure) FString GetObjective() const;

    const FFTMonsterSpecies* FindSpecies(FName Id) const;
    const FinalTowerRules::Battle& GetBattle() const { return Battle; }
    const FFTMonsterSpecies* GetEnemySpecies() const { return FindSpecies(EnemyId); }

private:
    FinalTowerRules::Battle Battle;
    FName EnemyId;
    TArray<FString> DialogueLines;
    int32 DialogueIndex = 0;
    bool bFinishStoryAfterDialogue = false;
    void AddFriend(FName SpeciesId);
    void FinishAction();
    void HealFriends();
};
