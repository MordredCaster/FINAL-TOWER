#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "FTSpriteRules.h"
#include "FTSpriteAnimator.generated.h"

class UPaperFlipbook;
class UPaperFlipbookComponent;
class USceneComponent;
class UStaticMeshComponent;

UCLASS(ClassGroup=(FinalTower), meta=(BlueprintSpawnableComponent))
class FINALTOWER_API UFTSpriteAnimator : public UActorComponent
{
    GENERATED_BODY()
public:
    UFTSpriteAnimator();
    bool ConfigureVisual(FName Id, UPaperFlipbookComponent* Component);
    void SetLocomotion(bool bMoving, const FVector& WorldVelocity);
    void FaceWorldDirection(const FVector& Delta);
    bool PlayAction(FName Action, float HoldSeconds = 0.0f, bool bLockMovement = false);
    bool IsMovementLocked() const { return ForcedRemaining > 0 && bForcedLocksMovement; }
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    UPROPERTY() TObjectPtr<UPaperFlipbookComponent> Target;
    UPROPERTY() TMap<FString, TObjectPtr<UPaperFlipbook>> Clips;
    TSet<FString> Missing;
    FName VisualId;
    FName Locomotion = TEXT("idle");
    FName ForcedAction;
    float ForcedRemaining = 0;
    bool bForcedLocksMovement = false;
    FinalTowerSpriteRules::Facing Facing = FinalTowerSpriteRules::Facing::South;
    UPaperFlipbook* FindClip(FName Action);
    bool Refresh();
};

/** Animated monster in the exploration scene; interactions stay in the game mode. */
UCLASS()
class FINALTOWER_API AFTMonsterVisual : public AActor
{
    GENERATED_BODY()
public:
    AFTMonsterVisual();
    bool Configure(FName Id);
    void FacePlayer(const FVector& PlayerPosition);
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> SceneRoot;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPaperFlipbookComponent> Sprite;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UFTSpriteAnimator> Animator;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Placeholder;
};
