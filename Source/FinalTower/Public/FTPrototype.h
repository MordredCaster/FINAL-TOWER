#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "FTPrototype.generated.h"

class UBoxComponent;
class UFloatingPawnMovement;
class UPaperFlipbookComponent;
class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class UPawnMovementComponent;
class UFTSpriteAnimator;
class AFTMonsterVisual;
class UTexture2D;

/** Floating top-down pawn with a replaceable 2D visual in a 3D scene. */
UCLASS()
class FINALTOWER_API AFTExplorer : public APawn
{
    GENERATED_BODY()
public:
    AFTExplorer();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual UPawnMovementComponent* GetMovementComponent() const override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UBoxComponent> Body;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UFloatingPawnMovement> Movement;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UPaperFlipbookComponent> PixelVisual;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UFTSpriteAnimator> SpriteAnimator;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> Placeholder;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USpringArmComponent> CameraArm;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UCameraComponent> Camera;
};

UCLASS()
class FINALTOWER_API AFTPrototypeController : public APlayerController
{
    GENERATED_BODY()
public:
    AFTPrototypeController();
    virtual void PlayerTick(float DeltaTime) override;
    void Interact();
    void BattleAction(FName Action);
    bool IsDragging() const { return bDragging; }
    FVector2D GetDragOrigin() const { return DragOrigin; }
    FVector2D GetDragPosition() const { return DragPosition; }

private:
    bool bPointerWasDown = false;
    bool bDragging = false;
    FVector2D DragOrigin = FVector2D::ZeroVector;
    FVector2D DragPosition = FVector2D::ZeroVector;
};

UCLASS()
class FINALTOWER_API AFTPrototypeGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AFTPrototypeGameMode();
    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    void InteractAt(const FVector& Position);

    static FVector KeeperPosition() { return FVector(-220, -280, 50); }
    static FVector EncounterPosition() { return FVector(420, 320, 50); }
    static FVector RoncinetPosition() { return FVector(380, -120, 0); }

private:
    UPROPERTY() TObjectPtr<AActor> PrototypeStart;
    UPROPERTY() TObjectPtr<AActor> EncounterMarker;
    UPROPERTY() TObjectPtr<AActor> LanternMarker;
    UPROPERTY() TObjectPtr<AFTMonsterVisual> Roncinet;
    UPROPERTY() TObjectPtr<AFTMonsterVisual> Companion;
    AActor* MakeShape(const TCHAR* MeshPath, const FVector& Location, const FVector& Scale, bool bCollision = false);
};

UCLASS()
class FINALTOWER_API AFTPrototypeHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
    bool HandleTap(FVector2D Position);
    bool IsArtBookOpen() const { return bArtBookOpen; }

private:
    struct FButton
    {
        FName Action;
        FVector2D Position;
        FVector2D Size;
    };
    TArray<FButton> Buttons;
    UPROPERTY() TArray<TObjectPtr<UTexture2D>> ArtPages;
    bool bArtBookOpen = false;
    int32 ArtPage = 0;
    float Scale = 1.0f;
    float ViewWidth = 1280.0f;
    float ViewHeight = 720.0f;
    void Label(const FString& Text, float X, float Y, FLinearColor Color = FLinearColor::White, float TextScale = 1.0f);
    void Wrapped(const FString& Text, float X, float Y, float Width);
    void Button(FName Action, const FString& Text, float X, float Y, float Width);
    void Act(FName Action);
    void DrawArtBook();
};
