#include "FTSpriteAnimator.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogFinalTowerSprites, Log, All);

UFTSpriteAnimator::UFTSpriteAnimator()
{
    PrimaryComponentTick.bCanEverTick = true;
}

bool UFTSpriteAnimator::ConfigureVisual(FName Id, UPaperFlipbookComponent* Component)
{
    if (Target == Component && VisualId == Id) return Target && Target->GetFlipbook();
    Target = Component;
    VisualId = Id;
    Clips.Reset();
    Missing.Reset();
    ForcedRemaining = 0;
    ForcedAction = NAME_None;
    Locomotion = TEXT("idle");
    if (Target) Target->SetFlipbook(nullptr);
    return Refresh();
}

UPaperFlipbook* UFTSpriteAnimator::FindClip(FName Action)
{
    const FString Direction = UTF8_TO_TCHAR(FinalTowerSpriteRules::DirectionName(Facing));
    const FString Key = Action.ToString() + TEXT("_") + Direction;
    if (const TObjectPtr<UPaperFlipbook>* Found = Clips.Find(Key)) return Found->Get();
    if (Missing.Contains(Key) || VisualId.IsNone()) return nullptr;
    const FString Base = FString::Printf(TEXT("/Game/GeneratedArt/%s/%s/FB_%s_%s_%s"),
        *VisualId.ToString(), *Action.ToString(), *VisualId.ToString(), *Action.ToString(), *Direction);
    UPaperFlipbook* Clip = LoadObject<UPaperFlipbook>(nullptr, *(Base + TEXT(".") + FPaths::GetCleanFilename(Base)));
    if (Clip) Clips.Add(Key, Clip);
    else
    {
        Missing.Add(Key);
        UE_LOG(LogFinalTowerSprites, Warning, TEXT("Missing sprite animation: %s. Open the Editor to build GeneratedArt."), *Base);
    }
    return Clip;
}

bool UFTSpriteAnimator::Refresh()
{
    if (!Target) return false;
    const FName Action = ForcedRemaining > 0 ? ForcedAction : Locomotion;
    UPaperFlipbook* Clip = FindClip(Action);
    if (!Clip && Action != TEXT("idle")) Clip = FindClip(TEXT("idle"));
    if (!Clip) return false;
    if (Target->GetFlipbook() != Clip)
    {
        Target->SetFlipbook(Clip);
        Target->SetLooping(Action == TEXT("idle") || Action == TEXT("walk") || Action == TEXT("sit"));
        Target->PlayFromStart();
    }
    return true;
}

void UFTSpriteAnimator::FaceWorldDirection(const FVector& Delta)
{
    const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);
    if (!Camera) return;
    const FRotationMatrix Axes(FRotator(0, Camera->GetCameraRotation().Yaw, 0));
    Facing = FinalTowerSpriteRules::FromScreenAxes(FVector::DotProduct(Delta, Axes.GetUnitAxis(EAxis::Y)),
        FVector::DotProduct(Delta, Axes.GetUnitAxis(EAxis::X)), Facing);
}

void UFTSpriteAnimator::SetLocomotion(bool bMoving, const FVector& WorldVelocity)
{
    // Monster walk sheets do not exist yet: their visual intentionally uses idle only.
    Locomotion = bMoving && VisualId == TEXT("hero") ? TEXT("walk") : TEXT("idle");
    if (ForcedRemaining <= 0) FaceWorldDirection(WorldVelocity);
    Refresh();
}

bool UFTSpriteAnimator::PlayAction(FName Action, float HoldSeconds, bool bLockMovement)
{
    UPaperFlipbook* Clip = FindClip(Action);
    if (!Clip || !Target) return false;
    ForcedAction = Action;
    ForcedRemaining = FMath::Max(Clip->GetTotalDuration(), HoldSeconds);
    bForcedLocksMovement = bLockMovement;
    Target->SetFlipbook(Clip);
    Target->SetLooping(Action == TEXT("idle") || Action == TEXT("walk") || Action == TEXT("sit"));
    Target->PlayFromStart();
    return true;
}

void UFTSpriteAnimator::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* Function)
{
    Super::TickComponent(DeltaTime, TickType, Function);
    if (!Target) return;
    if (ForcedRemaining > 0)
    {
        ForcedRemaining = FMath::Max(0.0f, ForcedRemaining - DeltaTime);
        if (ForcedRemaining <= 0) ForcedAction = NAME_None;
    }
    if (const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
        // Local sprite X follows screen right and local Z follows screen up: no mirroring.
        Target->SetWorldRotation(FRotator(0, Camera->GetCameraRotation().Yaw + 90, -Camera->GetCameraRotation().Pitch));
    Refresh();
}

AFTMonsterVisual::AFTMonsterVisual()
{
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(SceneRoot);
    Sprite = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Sprite"));
    Sprite->SetupAttachment(SceneRoot);
    Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Sprite->SetCastShadow(false);
    Animator = CreateDefaultSubobject<UFTSpriteAnimator>(TEXT("Animator"));
    Placeholder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MissingArtMarker"));
    Placeholder->SetupAttachment(SceneRoot);
    Placeholder->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Placeholder->SetRelativeLocation(FVector(0, 0, 25));
    Placeholder->SetRelativeScale3D(FVector(0.5));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (Mesh.Succeeded()) Placeholder->SetStaticMesh(Mesh.Object);
}

bool AFTMonsterVisual::Configure(FName Id)
{
    const bool bReady = Animator->ConfigureVisual(Id, Sprite);
    Placeholder->SetVisibility(!bReady);
    return bReady;
}

void AFTMonsterVisual::FacePlayer(const FVector& Position)
{
    Animator->FaceWorldDirection((Position - GetActorLocation()).GetSafeNormal2D());
}
