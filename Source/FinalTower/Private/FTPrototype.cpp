#include "FTPrototype.h"
#include "FTAdventureSubsystem.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/Canvas.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "PaperFlipbookComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
UFTAdventureSubsystem* Adventure(const UObject* Context)
{
    UGameInstance* Instance = UGameplayStatics::GetGameInstance(Context);
    return Instance ? Instance->GetSubsystem<UFTAdventureSubsystem>() : nullptr;
}
}

AFTExplorer::AFTExplorer()
{
    PrimaryActorTick.bCanEverTick = true;
    Body = CreateDefaultSubobject<UBoxComponent>(TEXT("Body"));
    Body->InitBoxExtent(FVector(18, 18, 34));
    Body->SetCollisionProfileName(TEXT("Pawn"));
    SetRootComponent(Body);

    Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
    Movement->UpdatedComponent = Body;
    Movement->MaxSpeed = 240.0f;
    Movement->Acceleration = 1800.0f;
    Movement->Deceleration = 2400.0f;
    Movement->SetPlaneConstraintNormal(FVector::UpVector);
    Movement->SetPlaneConstraintEnabled(true);

    PixelVisual = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("PixelVisual"));
    PixelVisual->SetupAttachment(Body);
    PixelVisual->SetRelativeLocation(FVector(0, 0, -32));
    PixelVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    Placeholder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Placeholder"));
    Placeholder->SetupAttachment(Body);
    Placeholder->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
    if (Mesh.Succeeded()) Placeholder->SetStaticMesh(Mesh.Object);
    Placeholder->SetRelativeScale3D(FVector(0.45, 0.45, 0.7));

    CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
    CameraArm->SetupAttachment(Body);
    CameraArm->TargetArmLength = 1250;
    CameraArm->SetRelativeRotation(FRotator(-55, -45, 0));
    CameraArm->bDoCollisionTest = false;
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(CameraArm);
    Camera->FieldOfView = 55;
}

void AFTExplorer::BeginPlay()
{
    Super::BeginPlay();
    Placeholder->SetVisibility(PixelVisual->GetFlipbook() == nullptr);
}

UPawnMovementComponent* AFTExplorer::GetMovementComponent() const
{
    return Movement;
}

void AFTExplorer::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    FVector Position = GetActorLocation();
    Position.X = FMath::Clamp(Position.X, -680.0, 680.0);
    Position.Y = FMath::Clamp(Position.Y, -680.0, 680.0);
    Position.Z = 40.0;
    SetActorLocation(Position);
    // A sprite occupies its local X/Z plane. Face its normal towards the fixed camera.
    if (PixelVisual->GetFlipbook())
    {
        const float CameraYaw = Camera->GetComponentRotation().Yaw;
        PixelVisual->SetWorldRotation(FRotator(0, CameraYaw + 90, 0));
    }
}

AFTPrototypeController::AFTPrototypeController()
{
    bShowMouseCursor = true;
    bEnableClickEvents = false; // One input path for touch and mouse prevents double activation.
    bEnableTouchEvents = false;
}

void AFTPrototypeController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
    UFTAdventureSubsystem* State = Adventure(this);
    if (!State) return;

    float PointerX = 0, PointerY = 0;
    bool bDown = false;
    GetInputTouchState(ETouchIndex::Touch1, PointerX, PointerY, bDown);
    if (!bDown && IsInputKeyDown(EKeys::LeftMouseButton))
        bDown = GetMousePosition(PointerX, PointerY);
    const FVector2D Pointer(PointerX, PointerY);
    if (bDown && !bPointerWasDown)
    {
        AFTPrototypeHUD* UI = Cast<AFTPrototypeHUD>(GetHUD());
        const bool bConsumed = UI && UI->HandleTap(Pointer);
        bDragging = !bConsumed && !State->bInBattle && State->Dialogue.IsEmpty();
        DragOrigin = Pointer;
    }
    DragPosition = Pointer;
    if (!bDown) bDragging = false;
    bPointerWasDown = bDown;

    if (WasInputKeyJustPressed(EKeys::E) || WasInputKeyJustPressed(EKeys::SpaceBar)) Interact();
    if (State->bInBattle)
    {
        if (WasInputKeyJustPressed(EKeys::One)) State->Attack();
        if (WasInputKeyJustPressed(EKeys::Two)) State->Recruit();
        if (WasInputKeyJustPressed(EKeys::Three)) State->Escape();
    }

    APawn* Controlled = GetPawn();
    if (!Controlled) return;
    if (State->bInBattle || !State->Dialogue.IsEmpty())
    {
        if (UPawnMovementComponent* Move = Controlled->GetMovementComponent()) Move->StopMovementImmediately();
        return;
    }
    FVector2D Input(0, 0);
    Input.X = (IsInputKeyDown(EKeys::D) || IsInputKeyDown(EKeys::Right) ? 1.0 : 0.0)
            - (IsInputKeyDown(EKeys::A) || IsInputKeyDown(EKeys::Q) || IsInputKeyDown(EKeys::Left) ? 1.0 : 0.0);
    Input.Y = (IsInputKeyDown(EKeys::W) || IsInputKeyDown(EKeys::Z) || IsInputKeyDown(EKeys::Up) ? 1.0 : 0.0)
            - (IsInputKeyDown(EKeys::S) || IsInputKeyDown(EKeys::Down) ? 1.0 : 0.0);
    if (bDragging)
    {
        int32 Width = 0, Height = 0;
        GetViewportSize(Width, Height);
        const float Radius = FMath::Max(40.0f, static_cast<float>(Height) * 0.12f);
        const FVector2D Delta = (DragPosition - DragOrigin) / Radius;
        Input = FVector2D(Delta.X, -Delta.Y);
    }
    const double Length = Input.Size();
    if (Length > 1.0) Input /= Length;
    const FRotator Yaw(0, -45, 0);
    Controlled->AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Input.X);
    Controlled->AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Input.Y);
}

void AFTPrototypeController::Interact()
{
    UFTAdventureSubsystem* State = Adventure(this);
    if (!State || State->bInBattle) return;
    if (!State->Dialogue.IsEmpty()) { State->AdvanceDialogue(); return; }
    AFTPrototypeGameMode* Mode = Cast<AFTPrototypeGameMode>(UGameplayStatics::GetGameMode(this));
    if (Mode && GetPawn()) Mode->InteractAt(GetPawn()->GetActorLocation());
}

AFTPrototypeGameMode::AFTPrototypeGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
    DefaultPawnClass = AFTExplorer::StaticClass();
    PlayerControllerClass = AFTPrototypeController::StaticClass();
    HUDClass = AFTPrototypeHUD::StaticClass();
}

AActor* AFTPrototypeGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    // Entry has no game-specific PlayerStart. Provide one before the default pawn is spawned.
    if (!PrototypeStart)
    {
        FActorSpawnParameters Parameters;
        Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        PrototypeStart = GetWorld()->SpawnActor<APlayerStart>(FVector(0, 0, 40), FRotator::ZeroRotator, Parameters);
    }
    return PrototypeStart;
}

AActor* AFTPrototypeGameMode::MakeShape(const TCHAR* MeshPath, const FVector& Location, const FVector& Scale, bool bCollision)
{
    UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, MeshPath);
    if (!Mesh) return nullptr;
    AStaticMeshActor* Actor = GetWorld()->SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator);
    if (!Actor) return nullptr;
    UStaticMeshComponent* Component = Actor->GetStaticMeshComponent();
    Component->SetMobility(EComponentMobility::Movable);
    Component->SetStaticMesh(Mesh);
    Component->SetWorldScale3D(Scale);
    Component->SetCollisionEnabled(bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    return Actor;
}

void AFTPrototypeGameMode::BeginPlay()
{
    Super::BeginPlay();
    const TCHAR* Cube = TEXT("/Engine/BasicShapes/Cube.Cube");
    const TCHAR* Sphere = TEXT("/Engine/BasicShapes/Sphere.Sphere");
    const TCHAR* Cone = TEXT("/Engine/BasicShapes/Cone.Cone");
    const TCHAR* Cylinder = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
    // A native greybox is generated in Entry; no map binaries or paid assets are required.
    MakeShape(Cube, FVector(0, 0, -30), FVector(16, 16, 0.5), true);
    MakeShape(Cube, FVector(-150, -400, 50), FVector(2.6, 1.8, 1.0), true);
    MakeShape(Cone, FVector(-150, -400, 145), FVector(3.3, 2.4, 1.2));
    MakeShape(Cylinder, KeeperPosition(), FVector(0.45, 0.45, 1.0));
    LanternMarker = MakeShape(Sphere, KeeperPosition() + FVector(0, 75, 40), FVector(0.25));
    EncounterMarker = MakeShape(Sphere, EncounterPosition(), FVector(0.6));

    // Distant tower is only a visual landmark. The larger setting remains open for design.
    MakeShape(Cylinder, FVector(900, 650, 180), FVector(1.6, 1.6, 4.0));
    MakeShape(Cone, FVector(900, 650, 430), FVector(2.2, 2.2, 1.4));
    for (int32 Index = 0; Index < 8; ++Index)
    {
        const FVector Position(400 + (Index % 4) * 65, 450 + (Index / 4) * 80, 20);
        MakeShape(Cylinder, Position, FVector(0.06, 0.06, 0.7));
    }
    for (int32 Index = 0; Index < 5; ++Index)
        MakeShape(Cone, FVector(-600 + Index * 270, 680, 100), FVector(1.5, 1.5, 2.4));

    ADirectionalLight* Sun = GetWorld()->SpawnActor<ADirectionalLight>(FVector(0, 0, 600), FRotator(-40, -30, 0));
    if (Sun)
    {
        Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
        Sun->GetLightComponent()->SetIntensity(3.0f);
        Sun->GetLightComponent()->SetLightColor(FLinearColor(1.0f, 0.78f, 0.55f));
    }
    ASkyLight* Sky = GetWorld()->SpawnActor<ASkyLight>();
    if (Sky)
    {
        Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
        Sky->GetLightComponent()->SetIntensity(0.7f);
    }
}

void AFTPrototypeGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const UFTAdventureSubsystem* State = Adventure(this);
    if (!State) return;
    if (EncounterMarker) EncounterMarker->SetActorHiddenInGame(State->HasFriend(TEXT("luciolle")));
    if (LanternMarker)
        LanternMarker->SetActorScale3D(FVector(State->bStoryComplete ? 0.5 : 0.25));
}

void AFTPrototypeGameMode::InteractAt(const FVector& Position)
{
    UFTAdventureSubsystem* State = Adventure(this);
    if (!State || State->bInBattle) return;
    if (FVector::Dist2D(Position, KeeperPosition()) < 170)
        State->TalkToKeeper();
    else if (!State->HasFriend(TEXT("luciolle")) && FVector::Dist2D(Position, EncounterPosition()) < 170)
        State->BeginEncounter(TEXT("luciolle"));
    else
        State->Message = TEXT("Approchez de la gardienne ou de la lumière près des roseaux, puis touchez Interagir.");
}

void AFTPrototypeHUD::Label(const FString& Text, float X, float Y, FLinearColor Color, float TextScale)
{
    DrawText(Text, Color, X * Scale, Y * Scale, GEngine->GetMediumFont(), Scale * TextScale);
}

void AFTPrototypeHUD::Wrapped(const FString& Text, float X, float Y, float Width)
{
    // Measure actual glyph widths; French accents and different viewport sizes keep their layout.
    TArray<FString> Words;
    Text.ParseIntoArray(Words, TEXT(" "), true);
    FString Line;
    for (const FString& Word : Words)
    {
        const FString Candidate = Line.IsEmpty() ? Word : Line + TEXT(" ") + Word;
        float TextWidth = 0, TextHeight = 0;
        GetTextSize(Candidate, TextWidth, TextHeight, GEngine->GetMediumFont(), Scale);
        if (TextWidth > Width * Scale && !Line.IsEmpty())
        {
            Label(Line, X, Y);
            Y += 25;
            Line = Word;
        }
        else Line = Candidate;
    }
    if (!Line.IsEmpty()) Label(Line, X, Y);
}

void AFTPrototypeHUD::Button(FName Action, const FString& Text, float X, float Y, float Width)
{
    DrawRect(FLinearColor(0.1f, 0.2f, 0.26f, 0.96f), X * Scale, Y * Scale, Width * Scale, 54 * Scale);
    Label(Text, X + 12, Y + 15);
    Buttons.Add({ Action, FVector2D(X, Y) * Scale, FVector2D(Width, 54) * Scale });
}

void AFTPrototypeHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas || !GEngine) return;
    UFTAdventureSubsystem* State = Adventure(this);
    if (!State) return;
    Buttons.Reset();
    Scale = FMath::Max(0.1f, static_cast<float>(Canvas->SizeY) / 720.0f);
    ViewWidth = Canvas->SizeX / Scale;
    ViewHeight = 720;
    const float Margin = 40;
    DrawRect(FLinearColor(0.025f, 0.05f, 0.075f, 0.94f), 0, 0, Canvas->SizeX, 126 * Scale);
    Label(TEXT("FINAL TOWER — prototype 0.1"), Margin, 20, FLinearColor(1, 0.8, 0.4));
    Wrapped(State->GetObjective(), Margin, 55, ViewWidth - 2 * Margin);
    Label(TEXT("Visuels provisoires • sprites 2D / décors 3D à venir"), Margin, 96, FLinearColor(0.7, 0.75, 0.8), 0.8f);

    if (State->Friends.IsValidIndex(State->ActiveFriend))
    {
        const FFTMonsterFriend& Friend = State->Friends[State->ActiveFriend];
        const FFTMonsterSpecies* Entry = State->FindSpecies(Friend.SpeciesId);
        if (Entry)
        {
            const int32 HP = State->bInBattle ? State->GetBattle().Ally.HP : Friend.HP;
            Label(FString::Printf(TEXT("%s  %d/%d PV  |  Compagnons : %d"), *Entry->Name, HP, Entry->MaxHP, State->Friends.Num()), Margin, 145);
        }
    }

    if (!State->bInBattle && State->Dialogue.IsEmpty())
    {
        for (int32 Index = 0; Index < 2; ++Index)
        {
            if (Index == 1 && State->HasFriend(TEXT("luciolle"))) continue;
            const FVector World = Index == 0 ? AFTPrototypeGameMode::KeeperPosition() : AFTPrototypeGameMode::EncounterPosition();
            FVector2D Screen;
            if (PlayerOwner && PlayerOwner->ProjectWorldLocationToScreen(World + FVector(0, 0, 80), Screen))
                Label(Index == 0 ? TEXT("Gardienne") : TEXT("Lumière des roseaux"), Screen.X / Scale, Screen.Y / Scale);
        }
        Label(TEXT("Glisser pour marcher • clavier : ZQSD / WASD • E : interagir"), Margin, 185, FLinearColor(0.8, 0.85, 0.9), 0.85f);
    }

    const float PanelY = 440;
    DrawRect(FLinearColor(0.025f, 0.05f, 0.075f, 0.96f), 16 * Scale, PanelY * Scale,
             (ViewWidth - 32) * Scale, (ViewHeight - PanelY - 16) * Scale);
    if (!State->Dialogue.IsEmpty())
    {
        Wrapped(State->Dialogue, Margin, PanelY + 24, ViewWidth - 2 * Margin);
        Button(TEXT("interact"), TEXT("Suite"), ViewWidth - 210, 625, 170);
    }
    else if (State->bInBattle)
    {
        const auto& Battle = State->GetBattle();
        const FFTMonsterSpecies* Enemy = State->GetEnemySpecies();
        Label(FString::Printf(TEXT("%s  %d/%d PV — Amitié : %.0f %%"), Enemy ? *Enemy->Name : TEXT("Créature"),
            Battle.Enemy.HP, Battle.Enemy.MaxHP, Battle.RecruitmentChance() * 100), Margin, PanelY + 18, FLinearColor(1, 0.8, 0.4));
        Wrapped(State->Message, Margin, PanelY + 55, ViewWidth - 2 * Margin);
        const float Width = FMath::Min(240.0f, (ViewWidth - 112) / 3);
        Button(TEXT("attack"), TEXT("1. Attaquer"), Margin, 625, Width);
        Button(TEXT("recruit"), TEXT("2. Tendre la main"), Margin + Width + 16, 625, Width);
        Button(TEXT("escape"), TEXT("3. Reculer"), Margin + 2 * (Width + 16), 625, Width);
    }
    else
    {
        Wrapped(State->Message, Margin, PanelY + 24, ViewWidth - 2 * Margin);
        Button(TEXT("companion"), TEXT("Compagnon"), Margin, 625, 190);
        Button(TEXT("save"), TEXT("Sauver"), Margin + 206, 625, 145);
        Button(TEXT("interact"), TEXT("Interagir"), ViewWidth - 230, 625, 190);
    }
    if (const AFTPrototypeController* Control = Cast<AFTPrototypeController>(PlayerOwner))
    {
        if (Control->IsDragging() && !State->bInBattle && State->Dialogue.IsEmpty())
        {
            const FVector2D Origin = Control->GetDragOrigin();
            const FVector2D Current = Control->GetDragPosition();
            DrawLine(Origin.X, Origin.Y, Current.X, Current.Y, FLinearColor(0.9, 0.75, 0.4), 4 * Scale);
        }
    }
}

bool AFTPrototypeHUD::HandleTap(FVector2D Position)
{
    for (const FButton& Entry : Buttons)
    {
        if (Position.X >= Entry.Position.X && Position.Y >= Entry.Position.Y &&
            Position.X <= Entry.Position.X + Entry.Size.X && Position.Y <= Entry.Position.Y + Entry.Size.Y)
        {
            Act(Entry.Action);
            return true;
        }
    }
    return false;
}

void AFTPrototypeHUD::Act(FName Action)
{
    UFTAdventureSubsystem* State = Adventure(this);
    if (!State) return;
    if (Action == TEXT("interact"))
    {
        if (AFTPrototypeController* Control = Cast<AFTPrototypeController>(PlayerOwner)) Control->Interact();
    }
    else if (Action == TEXT("attack")) State->Attack();
    else if (Action == TEXT("recruit")) State->Recruit();
    else if (Action == TEXT("escape")) State->Escape();
    else if (Action == TEXT("companion")) State->NextCompanion();
    else if (Action == TEXT("save"))
        State->Message = State->SaveProgress() ? TEXT("Compagnons et histoire sauvegardés.") : TEXT("Sauvegarde impossible. Réessayez.");
}
