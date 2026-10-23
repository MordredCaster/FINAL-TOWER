#include "FTPrototype.h"
#include "FTAdventureSubsystem.h"
#include "FTSpriteAnimator.h"
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
#include "Engine/Texture2D.h"
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
    PixelVisual->SetRelativeLocation(FVector(0, 0, -40));
    PixelVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PixelVisual->SetCastShadow(false);
    SpriteAnimator = CreateDefaultSubobject<UFTSpriteAnimator>(TEXT("SpriteAnimator"));

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
    const bool bReady = SpriteAnimator->ConfigureVisual(TEXT("hero"), PixelVisual);
    Placeholder->SetVisibility(!bReady);
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
    const UFTAdventureSubsystem* State = Adventure(this);
    const bool bWalking = GetVelocity().SizeSquared2D() > 4 && State && !State->bInBattle && State->Dialogue.IsEmpty();
    SpriteAnimator->SetLocomotion(bWalking, GetVelocity().GetSafeNormal2D());
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

    AFTPrototypeHUD* UI = Cast<AFTPrototypeHUD>(GetHUD());
    const bool bTitleWasOpen = !State->HasStartedGame() || (UI && UI->IsMainMenuOpen());

    float PointerX = 0, PointerY = 0;
    bool bDown = false;
    GetInputTouchState(ETouchIndex::Touch1, PointerX, PointerY, bDown);
    if (!bDown && IsInputKeyDown(EKeys::LeftMouseButton))
        bDown = GetMousePosition(PointerX, PointerY);
    const FVector2D Pointer(PointerX, PointerY);
    if (bDown && !bPointerWasDown)
    {
        const bool bConsumed = UI && UI->HandleTap(Pointer);
        bDragging = !bTitleWasOpen && !bConsumed && !State->bInBattle && State->Dialogue.IsEmpty();
        DragOrigin = Pointer;
    }
    DragPosition = Pointer;
    if (!bDown) bDragging = false;
    bPointerWasDown = bDown;

    if (bTitleWasOpen)
    {
        if (UI && UI->IsMainMenuOpen()) UI->HandleMainMenuKeys();
        if (GetPawn() && GetPawn()->GetMovementComponent()) GetPawn()->GetMovementComponent()->StopMovementImmediately();
        bDragging = false;
        return; // Starting a session never forwards this tap/key to gameplay.
    }
    const AFTExplorer* Explorer = Cast<AFTExplorer>(GetPawn());
    if (WasInputKeyJustPressed(EKeys::I) && UI && State->Dialogue.IsEmpty() && !State->bAwaitingReplacement)
        UI->ToggleInventory();
    if (WasInputKeyJustPressed(EKeys::J) && UI && State->Dialogue.IsEmpty() && !State->bAwaitingReplacement)
        UI->ToggleJournal();
    if ((UI && UI->IsMenuOpen()) || (Explorer && Explorer->SpriteAnimator->IsMovementLocked()))
    {
        if (GetPawn() && GetPawn()->GetMovementComponent()) GetPawn()->GetMovementComponent()->StopMovementImmediately();
        bDragging = false;
        return;
    }

    if (WasInputKeyJustPressed(EKeys::E) || WasInputKeyJustPressed(EKeys::SpaceBar)) Interact();
    if (State->bInBattle)
    {
        if (WasInputKeyJustPressed(EKeys::One)) BattleAction(TEXT("attack"));
        if (WasInputKeyJustPressed(EKeys::Two)) BattleAction(TEXT("recruit"));
        if (WasInputKeyJustPressed(EKeys::Three)) BattleAction(TEXT("escape"));
        if (WasInputKeyJustPressed(EKeys::Four)) BattleAction(TEXT("skill_0"));
        if (WasInputKeyJustPressed(EKeys::Five)) BattleAction(TEXT("skill_1"));
        if (WasInputKeyJustPressed(EKeys::Six)) BattleAction(TEXT("skill_2"));
    }
    if (WasInputKeyJustPressed(EKeys::Tab) && UI && State->Dialogue.IsEmpty()) UI->OpenTeam();

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
    if (!State || !State->HasStartedGame() || State->bInBattle) return;
    if (!State->Dialogue.IsEmpty()) { State->AdvanceDialogue(); return; }
    AFTPrototypeGameMode* Mode = Cast<AFTPrototypeGameMode>(UGameplayStatics::GetGameMode(this));
    if (Mode && GetPawn()) Mode->InteractAt(GetPawn()->GetActorLocation());
}

void AFTPrototypeController::BattleAction(FName Action)
{
    UFTAdventureSubsystem* State = Adventure(this);
    if (!State || !State->bInBattle || State->bAwaitingReplacement) return;
    AFTExplorer* Explorer = Cast<AFTExplorer>(GetPawn());
    if (Action == TEXT("attack"))
    {
        if (Explorer) Explorer->SpriteAnimator->PlayAction(TEXT("combat"));
        State->Attack();
    }
    else if (Action == TEXT("recruit"))
    {
        if (Explorer) Explorer->SpriteAnimator->PlayAction(TEXT("spell"));
        State->Recruit();
    }
    else if (Action == TEXT("escape")) State->Escape();
    else if (Action.ToString().StartsWith(TEXT("skill_")))
    {
        const int32 Index = FCString::Atoi(*Action.ToString().RightChop(6));
        if (State->UseSkill(Index) && Explorer)
        {
            const bool Physical = State->Friends[State->ActiveFriend].SpeciesId == TEXT("roncinet") && Index != 1;
            Explorer->SpriteAnimator->PlayAction(Physical ? TEXT("combat") : TEXT("spell"));
        }
    }
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
    AFTMonsterVisual* Luciolle = GetWorld()->SpawnActor<AFTMonsterVisual>(EncounterPosition(), FRotator::ZeroRotator);
    if (Luciolle) Luciolle->Configure(TEXT("luciolle"));
    EncounterMarker = Luciolle;
    Roncinet = GetWorld()->SpawnActor<AFTMonsterVisual>(RoncinetPosition(), FRotator::ZeroRotator);
    if (Roncinet) Roncinet->Configure(TEXT("roncinet"));
    Companion = GetWorld()->SpawnActor<AFTMonsterVisual>(FVector(-100, 80, 0), FRotator::ZeroRotator);
    if (Companion) Companion->Configure(TEXT("mousse"));

    WoundedMousse = GetWorld()->SpawnActor<AFTMonsterVisual>(WoundedPosition(), FRotator::ZeroRotator);
    if (WoundedMousse) { WoundedMousse->Configure(TEXT("mousse")); WoundedMousse->SetActorScale3D(FVector(0.8)); }
    for (const auto& Entry : FinalTowerRules::DiscoveryCatalog())
    {
        const bool Chest = Entry.Kind == FinalTowerRules::DiscoveryKind::Chest;
        const FVector Size = Chest ? FVector(0.65, 0.50, 0.40) :
            (Entry.Kind == FinalTowerRules::DiscoveryKind::Plant ? FVector(0.32, 0.32, 0.48) : FVector(0.65, 0.65, 0.14));
        DiscoveryMarkers.Add(MakeShape(Chest ? Cube : Sphere, FVector(Entry.X, Entry.Y, Entry.Z), Size));
    }

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
    if (!State || !State->HasStartedGame()) return;
    if (bWasInBattle && !State->bInBattle &&
        (State->GetBattle().Result == FinalTowerRules::Outcome::Victory || State->GetBattle().Result == FinalTowerRules::Outcome::Recruited))
    {
        if (State->GetEncounterId() == TEXT("luciolle")) LuciolleRespawnAt = GetWorld()->GetTimeSeconds() + 12;
        if (State->GetEncounterId() == TEXT("roncinet")) RoncinetRespawnAt = GetWorld()->GetTimeSeconds() + 12;
    }
    bWasInBattle = State->bInBattle;
    if (EncounterMarker) EncounterMarker->SetActorHiddenInGame(!IsEncounterAvailable(TEXT("luciolle")));
    if (Roncinet) Roncinet->SetActorHiddenInGame(!IsEncounterAvailable(TEXT("roncinet")));
    if (const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
    {
        if (AFTMonsterVisual* Light = Cast<AFTMonsterVisual>(EncounterMarker)) Light->FacePlayer(Player->GetActorLocation());
        if (Roncinet) Roncinet->FacePlayer(Player->GetActorLocation());
        if (Companion && State->Friends.IsValidIndex(State->ActiveFriend))
        {
            const FName Id = State->Friends[State->ActiveFriend].SpeciesId;
            Companion->Configure(Id);
            // Until monster walk sheets exist, use an idle companion next to the player.
            const FVector Position = Player->GetActorLocation() + FVector(-70, -70, Id == TEXT("luciolle") ? -10 : -40);
            Companion->SetActorLocation(Position);
            Companion->FacePlayer(Player->GetActorLocation());
        }
    }
    for (int32 Index = 0; Index < DiscoveryMarkers.Num(); ++Index)
    {
        AActor* Marker = DiscoveryMarkers[Index];
        if (!Marker || Index >= FinalTowerRules::DiscoveryCount) continue;
        const auto& Entry = FinalTowerRules::DiscoveryCatalog()[Index];
        const bool Collected = State->IsDiscoveryCollected(FName(UTF8_TO_TCHAR(Entry.Key)));
        Marker->SetActorHiddenInGame(Collected && Entry.Kind == FinalTowerRules::DiscoveryKind::Plant);
        if (Entry.Kind == FinalTowerRules::DiscoveryKind::Chest)
        {
            // Collected chests stay visible as flattened open boxes in this greybox.
            Marker->SetActorScale3D(FVector(0.65, 0.50, Collected ? 0.12 : 0.40));
            Marker->SetActorLocation(FVector(Entry.X, Entry.Y, Collected ? 6 : Entry.Z));
        }
    }
    if (WoundedMousse)
    {
        WoundedMousse->SetActorHiddenInGame(State->GetWoundedStage() == static_cast<int32>(FinalTowerRules::WoundedStep::Completed));
        const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
        if (Player)
        {
            WoundedMousse->SetActorLocation(State->IsWoundedFollowing() ? Player->GetActorLocation() + FVector(-110, 70, -40) : WoundedPosition());
            WoundedMousse->FacePlayer(Player->GetActorLocation());
        }
    }
    if (LanternMarker)
        LanternMarker->SetActorScale3D(FVector(State->bStoryComplete ? 0.5 : 0.25));
}

FVector AFTPrototypeGameMode::GetWoundedPosition() const
{
    return WoundedMousse ? WoundedMousse->GetActorLocation() : WoundedPosition();
}

FName AFTPrototypeGameMode::FindInteraction(const FVector& Position) const
{
    const UFTAdventureSubsystem* State = Adventure(this);
    if (!State || State->bInBattle || !State->Dialogue.IsEmpty()) return NAME_None;
    FName Nearest;
    float Distance = 145.0f;
    const auto Consider = [&Nearest, &Distance, &Position](FName Id, const FVector& Target)
    {
        const float Candidate = FVector::Dist2D(Position, Target);
        if (Candidate < Distance) { Distance = Candidate; Nearest = Id; }
    };
    Consider(TEXT("keeper"), KeeperPosition());
    if (IsEncounterAvailable(TEXT("luciolle"))) Consider(TEXT("luciolle"), EncounterPosition());
    if (IsEncounterAvailable(TEXT("roncinet"))) Consider(TEXT("roncinet"), RoncinetPosition());
    if (State->GetWoundedStage() != static_cast<int32>(FinalTowerRules::WoundedStep::Completed) && !State->IsWoundedFollowing())
        Consider(TEXT("wounded_mousse"), GetWoundedPosition());
    for (const auto& Entry : FinalTowerRules::DiscoveryCatalog())
    {
        const FName Id(UTF8_TO_TCHAR(Entry.Key));
        if (!State->IsDiscoveryCollected(Id)) Consider(Id, FVector(Entry.X, Entry.Y, Entry.Z));
    }
    return Nearest;
}

FString AFTPrototypeGameMode::GetInteractionPrompt(const FVector& Position) const
{
    const FName Id = FindInteraction(Position);
    if (Id == TEXT("keeper")) return TEXT("Parler à la gardienne");
    if (Id == TEXT("wounded_mousse"))
    {
        const UFTAdventureSubsystem* State = Adventure(this);
        if (State && State->GetWoundedStage() == static_cast<int32>(FinalTowerRules::WoundedStep::Found))
            return TEXT("Soigner Mousse : 1 baie de braise");
        if (State && State->GetWoundedStage() == static_cast<int32>(FinalTowerRules::WoundedStep::Healed))
            return TEXT("Rassurer Mousse : 1 friandise des bois");
        return TEXT("Approcher la petite Mousse");
    }
    if (Id == TEXT("luciolle") || Id == TEXT("roncinet")) return TEXT("Rencontrer la créature");
    if (const auto* Entry = FinalTowerRules::FindDiscovery(TCHAR_TO_UTF8(*Id.ToString())))
        return FString::Printf(TEXT("%s : %s"), Entry->Kind == FinalTowerRules::DiscoveryKind::Plant ? TEXT("Récolter") : TEXT("Fouiller"), UTF8_TO_TCHAR(Entry->Name));
    return TEXT("Explorer les environs ; J : journal");
}

void AFTPrototypeGameMode::InteractAt(const FVector& Position)
{
    UFTAdventureSubsystem* State = Adventure(this);
    if (!State || State->bInBattle || !State->Dialogue.IsEmpty()) return;
    const FName Id = FindInteraction(Position);
    if (Id == TEXT("keeper")) State->TalkToKeeper();
    else if (Id == TEXT("wounded_mousse")) State->TalkToWounded();
    else if (Id == TEXT("luciolle") || Id == TEXT("roncinet")) State->BeginEncounter(Id);
    else if (!Id.IsNone()) State->CollectDiscovery(Id);
    else State->Message = TEXT("Approchez d'un coffre, d'une plante, d'une créature ou de la gardienne. J ouvre le journal.");
}

bool AFTPrototypeGameMode::IsEncounterAvailable(FName Id) const
{
    const float Time = GetWorld()->GetTimeSeconds();
    return Id == TEXT("luciolle") ? Time >= LuciolleRespawnAt : Time >= RoncinetRespawnAt;
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

void AFTPrototypeHUD::Button(FName Action, const FString& Text, float X, float Y, float Width, bool bEnabled)
{
    DrawRect(bEnabled ? FLinearColor(0.1f, 0.2f, 0.26f, 0.96f) : FLinearColor(0.10f, 0.11f, 0.12f, 0.96f),
        X * Scale, Y * Scale, Width * Scale, 54 * Scale);
    Label(Text, X + 12, Y + 15, bEnabled ? FLinearColor::White : FLinearColor(0.5f, 0.5f, 0.5f));
    if (bEnabled) Buttons.Add({ Action, FVector2D(X, Y) * Scale, FVector2D(Width, 54) * Scale });
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
    if (IsMainMenuOpen()) { DrawMainMenu(); return; }
    if (State->bAwaitingReplacement) { bTeamOpen = true; bInventoryOpen = bJournalOpen = false; }
    if (bTeamOpen) { DrawTeam(); return; }
    if (bInventoryOpen) { DrawInventory(); return; }
    if (bJournalOpen) { DrawJournal(); return; }
    if (bArtBookOpen) { DrawArtBook(); return; }
    const float Margin = 40;
    DrawRect(FLinearColor(0.025f, 0.05f, 0.075f, 0.94f), 0, 0, Canvas->SizeX, 126 * Scale);
    Label(TEXT("FINAL TOWER — prototype 0.6"), Margin, 20, FLinearColor(1, 0.8, 0.4));
    Wrapped(State->GetObjective(), Margin, 55, ViewWidth - 2 * Margin);
    Label(TEXT("J : journal de quêtes • Carnet : personnages et bestiaire"), Margin, 96, FLinearColor(0.7, 0.75, 0.8), 0.8f);

    if (State->Friends.IsValidIndex(State->ActiveFriend))
    {
        const FFTMonsterFriend& Friend = State->Friends[State->ActiveFriend];
        const FFTMonsterSpecies* Entry = State->FindSpecies(Friend.SpeciesId);
        if (Entry)
        {
            const FFTMonsterStats Stats = State->GetFriendStats(State->ActiveFriend);
            Label(FString::Printf(TEXT("%s niv. %d  |  PV %d/%d  PM %d/%d  |  XP %d/%d"),
                *Entry->Name, Stats.Level, Stats.HP, Stats.MaxHP, Stats.MP, Stats.MaxMP, Stats.Experience, Stats.NextLevelXP), Margin, 145);
        }
    }

    if (!State->bInBattle && State->Dialogue.IsEmpty())
    {
        for (int32 Index = 0; Index < 3; ++Index)
        {
            const AFTPrototypeGameMode* Mode = Cast<AFTPrototypeGameMode>(UGameplayStatics::GetGameMode(this));
            if (Mode && Index == 1 && !Mode->IsEncounterAvailable(TEXT("luciolle"))) continue;
            if (Mode && Index == 2 && !Mode->IsEncounterAvailable(TEXT("roncinet"))) continue;
            const FVector World = Index == 0 ? AFTPrototypeGameMode::KeeperPosition() :
                (Index == 1 ? AFTPrototypeGameMode::EncounterPosition() : AFTPrototypeGameMode::RoncinetPosition());
            FVector2D Screen;
            if (PlayerOwner && PlayerOwner->ProjectWorldLocationToScreen(World + FVector(0, 0, 80), Screen))
                Label(Index == 0 ? TEXT("Gardienne") : (Index == 1 ? TEXT("Luciolle") : TEXT("Roncinet")), Screen.X / Scale, Screen.Y / Scale);
        }
        Label(TEXT("Glisser pour marcher • ZQSD / WASD • E : interagir • J : journal"), Margin, 185, FLinearColor(0.8, 0.85, 0.9), 0.85f);
        const AFTPrototypeGameMode* Mode = Cast<AFTPrototypeGameMode>(UGameplayStatics::GetGameMode(this));
        const APawn* Player = PlayerOwner ? PlayerOwner->GetPawn() : nullptr;
        if (Mode && Player)
        {
            Label(Mode->GetInteractionPrompt(Player->GetActorLocation()), Margin, 220, FLinearColor(1, 0.85f, 0.5f));
            for (const auto& Entry : FinalTowerRules::DiscoveryCatalog())
            {
                if (State->IsDiscoveryCollected(FName(UTF8_TO_TCHAR(Entry.Key)))) continue;
                const FVector World(Entry.X, Entry.Y, Entry.Z + 60);
                FVector2D Screen;
                if (FVector::Dist2D(Player->GetActorLocation(), World) < 340 &&
                    PlayerOwner->ProjectWorldLocationToScreen(World, Screen))
                    Label(UTF8_TO_TCHAR(Entry.Name), Screen.X / Scale, Screen.Y / Scale, FLinearColor(1, 0.85f, 0.5f), 0.8f);
            }
            if (State->GetWoundedStage() != static_cast<int32>(FinalTowerRules::WoundedStep::Completed))
            {
                const FVector World = Mode->GetWoundedPosition() + FVector(0, 0, 95);
                FVector2D Screen;
                if (FVector::Dist2D(Player->GetActorLocation(), World) < 340 && PlayerOwner->ProjectWorldLocationToScreen(World, Screen))
                    Label(State->IsWoundedFollowing() ? TEXT("Mousse vous suit") : TEXT("Petite Mousse"), Screen.X / Scale, Screen.Y / Scale, FLinearColor(0.8f, 1, 0.7f), 0.8f);
            }
        }
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
        Label(FString::Printf(TEXT("%s niv. %d  %d/%d PV — Amitié : %.0f %%"), Enemy ? *Enemy->Name : TEXT("Créature"),
            State->GetEnemyLevel(), Battle.Enemy.HP, Battle.Enemy.MaxHP, Battle.RecruitmentChance() * 100), Margin, PanelY + 18, FLinearColor(1, 0.8, 0.4));
        Wrapped(State->Message, Margin, PanelY + 55, ViewWidth - 2 * Margin);
        Label(FString::Printf(TEXT("Protection : %d  |  Ennemi affaibli : %d  |  Friandise : %s"), Battle.GuardTurns, Battle.WeakenTurns,
            Battle.Fed ? TEXT("+20 points") : TEXT("non donnée")),
            Margin, 540, FLinearColor(0.7f, 0.85f, 0.7f), 0.85f);
        const auto Skills = State->GetTechniques(State->ActiveFriend);
        const float SkillWidth = FMath::Min(280.0f, (ViewWidth - 112) / 3);
        for (int32 Index = 0; Index < Skills.Num(); ++Index)
        {
            const auto& Skill = Skills[Index];
            const FString Text = Skill.bLearned ? FString::Printf(TEXT("%d. %s · %d PM"), Index + 4, *Skill.Name, Skill.MPCost) :
                FString::Printf(TEXT("Niv. %d : %s"), Skill.UnlockLevel, *Skill.Name);
            Button(FName(*FString::Printf(TEXT("skill_%d"), Index)), Text, Margin + Index * (SkillWidth + 16), 570, SkillWidth, Skill.bUsable);
        }
        const float Width = FMath::Min(220.0f, (ViewWidth - 144) / 5);
        Button(TEXT("attack"), TEXT("1. Attaquer"), Margin, 642, Width);
        Button(TEXT("recruit"), TEXT("2. Amitié"), Margin + Width + 16, 642, Width);
        Button(TEXT("escape"), TEXT("3. Reculer"), Margin + 2 * (Width + 16), 642, Width);
        Button(TEXT("team"), TEXT("Équipe · Tab"), Margin + 3 * (Width + 16), 642, Width);
        Button(TEXT("inventory"), TEXT("Sac · I"), Margin + 4 * (Width + 16), 642, Width);
    }
    else
    {
        Wrapped(State->Message, Margin, PanelY + 24, ViewWidth - 2 * Margin);
        const float Width = FMath::Min(185.0f, (ViewWidth - 160) / 6);
        Button(TEXT("companion"), TEXT("Équipe"), Margin, 625, Width);
        Button(TEXT("inventory"), TEXT("Sac · I"), Margin + Width + 16, 625, Width);
        Button(TEXT("save"), TEXT("Sauver"), Margin + 2 * (Width + 16), 625, Width);
        Button(TEXT("artbook"), TEXT("Carnet"), Margin + 3 * (Width + 16), 625, Width);
        Button(TEXT("journal"), TEXT("Journal"), Margin + 4 * (Width + 16), 625, Width);
        Button(TEXT("interact"), TEXT("Interagir"), Margin + 5 * (Width + 16), 625, Width);
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
            const FName Action = Entry.Action;
            Buttons.Reset(); // Do not retain hit targets from a page that just closed.
            Act(Action);
            return true;
        }
    }
    return IsMainMenuOpen(); // Blank space and disabled buttons cannot move the hero.
}

void AFTPrototypeHUD::Act(FName Action)
{
    UFTAdventureSubsystem* State = Adventure(this);
    if (!State) return;
    if (IsMainMenuOpen()) { MainMenuAction(Action); return; }
    if (Action == TEXT("interact"))
    {
        if (AFTPrototypeController* Control = Cast<AFTPrototypeController>(PlayerOwner)) Control->Interact();
    }
    else if (Action == TEXT("attack") || Action == TEXT("recruit") || Action == TEXT("escape") || Action.ToString().StartsWith(TEXT("skill_")))
    {
        if (AFTPrototypeController* Control = Cast<AFTPrototypeController>(PlayerOwner)) Control->BattleAction(Action);
    }
    else if (Action == TEXT("journal")) ToggleJournal();
    else if (Action == TEXT("closejournal")) bJournalOpen = false;
    else if (Action == TEXT("quest_0")) JournalQuest = 0;
    else if (Action == TEXT("quest_1")) JournalQuest = 1;
    else if (Action == TEXT("inventory")) ToggleInventory();
    else if (Action == TEXT("closebag")) bInventoryOpen = false;
    else if (Action == TEXT("bagback")) { SelectedItem = -1; InventoryPage = 0; }
    else if (Action == TEXT("bagprev")) InventoryPage = FMath::Max(0, InventoryPage - 1);
    else if (Action == TEXT("bagnext")) InventoryPage = FMath::Min((State->Friends.Num() - 1) / 4, InventoryPage + 1);
    else if (Action.ToString().StartsWith(TEXT("bagitem_")))
    {
        const int32 Index = FCString::Atoi(*Action.ToString().RightChop(8));
        if (Index < 0 || Index >= FinalTowerRules::ItemCount) return;
        const auto& Entry = FinalTowerRules::ItemCatalog()[Index];
        const FName Id(UTF8_TO_TCHAR(Entry.Key));
        if (State->GetItemQuantity(Id) <= 0) return;
        if (State->bInBattle || Entry.Effect == FinalTowerRules::ItemEffect::Befriend)
        {
            if (State->UseItem(Id, State->ActiveFriend)) bInventoryOpen = false;
        }
        else { SelectedItem = Index; InventoryPage = 0; }
    }
    else if (Action.ToString().StartsWith(TEXT("bagtarget_")))
    {
        if (SelectedItem < 0 || SelectedItem >= FinalTowerRules::ItemCount) return;
        const auto& Entry = FinalTowerRules::ItemCatalog()[SelectedItem];
        const int32 Index = FCString::Atoi(*Action.ToString().RightChop(10));
        State->UseItem(FName(UTF8_TO_TCHAR(Entry.Key)), Index);
    }
    else if (Action == TEXT("companion") || Action == TEXT("team")) OpenTeam();
    else if (Action == TEXT("closeteam")) bTeamOpen = false;
    else if (Action == TEXT("teamprev")) TeamPage = FMath::Max(0, TeamPage - 1);
    else if (Action == TEXT("teamnext")) TeamPage = FMath::Min((State->Friends.Num() - 1) / 5, TeamPage + 1);
    else if (Action.ToString().StartsWith(TEXT("lead_")))
    {
        const bool WasInBattle = State->bInBattle;
        if (State->SetActiveFriend(FCString::Atoi(*Action.ToString().RightChop(5))) && WasInBattle)
            bTeamOpen = State->bAwaitingReplacement;
    }
    else if (Action.ToString().StartsWith(TEXT("toggle_")))
        State->TogglePartyMember(FCString::Atoi(*Action.ToString().RightChop(7)));
    else if (Action == TEXT("save"))
        State->Message = State->SaveProgress() ? TEXT("Compagnons, sac, découvertes et quêtes sauvegardés.") : TEXT("Sauvegarde impossible. Réessayez.");
    else if (Action == TEXT("artbook")) { bArtBookOpen = true; ArtPage = 0; }
    else if (Action == TEXT("closebook")) bArtBookOpen = false;
    else if (Action == TEXT("nextpage")) ArtPage = (ArtPage + 1) % 5;
    else if (Action == TEXT("previouspage")) ArtPage = (ArtPage + 4) % 5;
    else if (Action.ToString().StartsWith(TEXT("preview_")))
    {
        if (AFTExplorer* Explorer = Cast<AFTExplorer>(PlayerOwner->GetPawn()))
        {
            const FName Clip(*Action.ToString().RightChop(8));
            const bool bPlayed = Explorer->SpriteAnimator->PlayAction(Clip, 3.0f, true);
            State->Message = bPlayed ? TEXT("Aperçu de l'animation pendant 3 secondes.") : TEXT("Animation absente : vérifier l'import GeneratedArt dans l'éditeur.");
            bArtBookOpen = false;
        }
    }
}

void AFTPrototypeHUD::ToggleInventory()
{
    if (IsMainMenuOpen()) return;
    const UFTAdventureSubsystem* State = Adventure(this);
    if (!State || !State->Dialogue.IsEmpty() || State->bAwaitingReplacement) return;
    bInventoryOpen = !bInventoryOpen;
    if (bInventoryOpen) { bTeamOpen = bJournalOpen = false; bArtBookOpen = false; SelectedItem = -1; InventoryPage = 0; }
}

void AFTPrototypeHUD::ToggleJournal()
{
    if (IsMainMenuOpen()) return;
    const UFTAdventureSubsystem* State = Adventure(this);
    if (!State || !State->Dialogue.IsEmpty() || State->bAwaitingReplacement) return;
    bJournalOpen = !bJournalOpen;
    if (bJournalOpen)
    {
        bTeamOpen = bInventoryOpen = bArtBookOpen = false;
        JournalQuest = State->GetWoundedStage() > 0 && State->GetWoundedStage() < 4 ? 1 : 0;
    }
}

void AFTPrototypeHUD::DrawJournal()
{
    const UFTAdventureSubsystem* State = Adventure(this);
    if (!State) return;
    const auto Quests = State->GetQuests();
    if (Quests.IsEmpty()) return;
    JournalQuest = FMath::Clamp(JournalQuest, 0, Quests.Num() - 1);
    const auto& Quest = Quests[JournalQuest];
    DrawRect(FLinearColor(0.025f, 0.035f, 0.05f, 1), 0, 0, Canvas->SizeX, Canvas->SizeY);
    Label(TEXT("JOURNAL — les histoires du village"), 40, 20, FLinearColor(1, 0.8f, 0.4f));
    Label(FString::Printf(TEXT("Découvertes : %d/%d · 2 coffres, 2 plantes et un nid"), State->GetDiscoveryCount(), FinalTowerRules::DiscoveryCount), 40, 62);
    const float Width = FMath::Min(380.0f, (ViewWidth - 96) / 2);
    Button(TEXT("quest_0"), TEXT("La lanterne"), 40, 105, Width);
    Button(TEXT("quest_1"), TEXT("Sous les branches"), 56 + Width, 105, Width);
    Label(Quest.Title, 40, 188, FLinearColor(1, 0.85f, 0.5f));
    Label(Quest.Status, 40, 224, Quest.bComplete ? FLinearColor(0.7f, 1, 0.7f) : FLinearColor::White);
    Label(TEXT("Objectif"), 40, 272, FLinearColor(0.75f, 0.85f, 1));
    Wrapped(Quest.Objective, 40, 305, ViewWidth - 80);
    Label(TEXT("Indices"), 40, 390, FLinearColor(0.75f, 0.85f, 1));
    Wrapped(Quest.Clues, 40, 423, ViewWidth - 80);
    Label(TEXT("Récompense"), 40, 505, FLinearColor(0.75f, 0.85f, 1));
    Wrapped(Quest.Reward, 40, 538, ViewWidth - 80);
    Button(TEXT("closejournal"), TEXT("Fermer · J"), ViewWidth - 210, 642, 170);
}

void AFTPrototypeHUD::DrawInventory()
{
    UFTAdventureSubsystem* State = Adventure(this);
    if (!State) return;
    DrawRect(FLinearColor(0.025f, 0.035f, 0.05f, 1), 0, 0, Canvas->SizeX, Canvas->SizeY);
    Label(TEXT("SAC — objets partagés"), 40, 20, FLinearColor(1, 0.8f, 0.4f));
    Wrapped(State->bInBattle ? TEXT("Un objet prend un tour et provoque une riposte. Les soins ciblent le compagnon actif.") :
        TEXT("Choisissez un soin puis un compagnon, y compris en réserve. Piles de 99 maximum."), 40, 62, ViewWidth - 80);
    if (State->bInBattle && State->Friends.IsValidIndex(State->ActiveFriend))
    {
        const auto Stats = State->GetFriendStats(State->ActiveFriend);
        const FFTMonsterSpecies* Species = State->FindSpecies(State->Friends[State->ActiveFriend].SpeciesId);
        Label(FString::Printf(TEXT("Actif : %s · PV %d/%d · PM %d/%d"), Species ? *Species->Name : TEXT("Compagnon"),
            Stats.HP, Stats.MaxHP, Stats.MP, Stats.MaxMP), 40, 96, FLinearColor(0.75f, 0.85f, 0.8f), 0.85f);
    }
    if (SelectedItem < 0 || SelectedItem >= FinalTowerRules::ItemCount)
    {
        int32 Row = 0;
        for (const auto& Entry : FinalTowerRules::ItemCatalog())
        {
            const float Y = 128 + Row * 126;
            const FName Id(UTF8_TO_TCHAR(Entry.Key));
            const int32 Quantity = State->GetItemQuantity(Id);
            Label(FString::Printf(TEXT("%s  × %d / 99"), UTF8_TO_TCHAR(Entry.Name), Quantity), 40, Y,
                Quantity > 0 ? FLinearColor::White : FLinearColor(0.5f, 0.5f, 0.5f));
            Wrapped(UTF8_TO_TCHAR(Entry.Description), 40, Y + 32, ViewWidth - 300);
            const bool Food = Entry.Effect == FinalTowerRules::ItemEffect::Befriend;
            const bool Enabled = State->bInBattle ? State->CanUseItem(Id, State->ActiveFriend) : Quantity > 0 && !Food;
            Button(FName(*FString::Printf(TEXT("bagitem_%d"), Row)),
                Food ? TEXT("Offrir") : (State->bInBattle ? TEXT("Utiliser") : TEXT("Choisir")), ViewWidth - 210, Y, 170, Enabled);
            ++Row;
        }
        if (!State->bInBattle) Label(TEXT("Les friandises s'offrent pendant une rencontre."), 40, 510, FLinearColor(0.75f, 0.8f, 0.85f), 0.85f);
    }
    else
    {
        const auto& Entry = FinalTowerRules::ItemCatalog()[SelectedItem];
        const FName Id(UTF8_TO_TCHAR(Entry.Key));
        Label(FString::Printf(TEXT("%s × %d — choisir la cible"), UTF8_TO_TCHAR(Entry.Name), State->GetItemQuantity(Id)), 40, 122);
        InventoryPage = FMath::Clamp(InventoryPage, 0, FMath::Max(0, (State->Friends.Num() - 1) / 4));
        for (int32 Row = 0; Row < 4; ++Row)
        {
            const int32 Index = InventoryPage * 4 + Row;
            if (!State->Friends.IsValidIndex(Index)) break;
            const auto Stats = State->GetFriendStats(Index);
            const FFTMonsterSpecies* Species = State->FindSpecies(State->Friends[Index].SpeciesId);
            if (!Species) continue;
            const float Y = 180 + Row * 80;
            Label(FString::Printf(TEXT("#%d %s N%d · %s"), Index + 1, *Species->Name, Stats.Level,
                State->Party.Contains(Index) ? TEXT("Équipe") : TEXT("Réserve")), 40, Y);
            Label(FString::Printf(TEXT("PV %d/%d  ·  PM %d/%d%s"), Stats.HP, Stats.MaxHP, Stats.MP, Stats.MaxMP,
                Stats.HP == 0 ? TEXT("  ·  K.-O.") : TEXT("")), 40, Y + 28, FLinearColor(0.75f, 0.8f, 0.85f), 0.85f);
            Button(FName(*FString::Printf(TEXT("bagtarget_%d"), Index)), TEXT("Soigner"), ViewWidth - 210, Y, 170, State->CanUseItem(Id, Index));
        }
        Label(FString::Printf(TEXT("Page %d / %d"), InventoryPage + 1, (State->Friends.Num() + 3) / 4), 40, 510);
        Button(TEXT("bagprev"), TEXT("Précédent"), 40, 642, 150, InventoryPage > 0);
        Button(TEXT("bagnext"), TEXT("Suivant"), 206, 642, 130, (InventoryPage + 1) * 4 < State->Friends.Num());
        Button(TEXT("bagback"), TEXT("Objets"), 352, 642, 130);
    }
    Wrapped(State->Message, 40, 550, ViewWidth - 80);
    Button(TEXT("closebag"), TEXT("Fermer · I"), ViewWidth - 210, 642, 170);
}

void AFTPrototypeHUD::DrawTeam()
{
    UFTAdventureSubsystem* State = Adventure(this);
    if (!State) return;
    DrawRect(FLinearColor(0.025f, 0.035f, 0.05f, 1), 0, 0, Canvas->SizeX, Canvas->SizeY);
    Label(FString::Printf(TEXT("ÉQUIPE %d/3 — collection : %d"), State->Party.Num(), State->Friends.Num()),
        40, 20, FLinearColor(1, 0.8f, 0.4f));
    Label(State->bInBattle ? TEXT("Un relais prend un tour. Après un K.-O., le remplacement est gratuit.") :
        TEXT("Trois membres maximum. Les autres attendent en réserve ; Tab ouvre ce menu."), 40, 62);
    TArray<int32> Displayed;
    if (State->bInBattle) Displayed = State->Party;
    else
    {
        TeamPage = FMath::Clamp(TeamPage, 0, FMath::Max(0, (State->Friends.Num() - 1) / 5));
        for (int32 Index = TeamPage * 5; Index < FMath::Min(State->Friends.Num(), TeamPage * 5 + 5); ++Index) Displayed.Add(Index);
    }
    for (int32 Row = 0; Row < Displayed.Num(); ++Row)
    {
        const int32 Index = Displayed[Row];
        const auto Stats = State->GetFriendStats(Index);
        const FFTMonsterSpecies* Species = State->FindSpecies(State->Friends[Index].SpeciesId);
        if (!Species) continue;
        const float Y = 110 + Row * 86;
        const bool Active = State->ActiveFriend == Index;
        const bool InParty = State->Party.Contains(Index);
        const FString Status = Active ? TEXT("Actif") : (InParty ? TEXT("Équipe") : TEXT("Réserve"));
        Label(FString::Printf(TEXT("%s · %s N%d  PV %d/%d  PM %d/%d"), *Status, *Species->Name,
            Stats.Level, Stats.HP, Stats.MaxHP, Stats.MP, Stats.MaxMP), 40, Y,
            Active ? FLinearColor(1, 0.85f, 0.5f) : FLinearColor::White);
        Label(FString::Printf(TEXT("%s · XP %d/%d · Att %d / Déf %d / Mag %d"), *Species->Role,
            Stats.Experience, Stats.NextLevelXP, Stats.Attack, Stats.Defense, Stats.Magic), 40, Y + 25,
            FLinearColor(0.75f, 0.8f, 0.85f), 0.8f);
        FString Skills;
        for (const auto& Skill : State->GetTechniques(Index))
        {
            if (!Skills.IsEmpty()) Skills += TEXT(" / ");
            Skills += Skill.bLearned ? Skill.Name : FString::Printf(TEXT("%s (N%d)"), *Skill.Name, Skill.UnlockLevel);
        }
        Label(Skills, 40, Y + 48, FLinearColor(0.7f, 0.85f, 0.7f), 0.75f);
        Button(FName(*FString::Printf(TEXT("lead_%d"), Index)), State->bInBattle ? TEXT("Relayer") : TEXT("En tête"),
            ViewWidth - 310, Y + 8, 120, InParty && !Active && Stats.HP > 0);
        if (!State->bInBattle)
            Button(FName(*FString::Printf(TEXT("toggle_%d"), Index)), InParty ? TEXT("Réserve") : TEXT("Ajouter"),
                ViewWidth - 174, Y + 8, 134, !InParty || State->Party.Num() > 1);
    }
    Wrapped(State->Message, 40, 558, ViewWidth - 80);
    if (!State->bInBattle)
    {
        Button(TEXT("teamprev"), TEXT("Précédent"), 40, 642, 170, TeamPage > 0);
        Button(TEXT("teamnext"), TEXT("Suivant"), 226, 642, 170, (TeamPage + 1) * 5 < State->Friends.Num());
    }
    if (!State->bAwaitingReplacement) Button(TEXT("closeteam"), TEXT("Fermer"), ViewWidth - 210, 642, 170);
}

void AFTPrototypeHUD::DrawArtBook()
{
    if (ArtPages.IsEmpty())
    {
        const TCHAR* Names[] = { TEXT("hero"), TEXT("brother"), TEXT("mousse"), TEXT("roncinet"), TEXT("luciolle") };
        for (const TCHAR* Name : Names)
        {
            const FString Path = FString::Printf(TEXT("/Game/GeneratedArt/Concepts/T_%s.T_%s"), Name, Name);
            ArtPages.Add(LoadObject<UTexture2D>(nullptr, *Path));
        }
    }
    DrawRect(FLinearColor(0.025f, 0.035f, 0.05f, 1), 0, 0, Canvas->SizeX, Canvas->SizeY);
    Label(TEXT("CARNET — personnages et bestiaire"), 40, 20, FLinearColor(1, 0.8, 0.4));
    if (ArtPages.IsValidIndex(ArtPage) && ArtPages[ArtPage])
    {
        UTexture2D* Page = ArtPages[ArtPage];
        const float Ratio = static_cast<float>(Page->GetSizeX()) / Page->GetSizeY();
        const float Width = FMath::Min(ViewWidth - 80, 480 * Ratio);
        const float Height = Width / Ratio;
        DrawTexture(Page, (ViewWidth - Width) * 0.5f * Scale, 65 * Scale, Width * Scale, Height * Scale,
            0, 0, 1, 1, FLinearColor::White, BLEND_Translucent);
    }
    else Wrapped(TEXT("Planche absente : ouvrir le projet dans l'éditeur pour générer les assets."), 40, 100, ViewWidth - 80);
    if (ArtPage == 0)
    {
        const TCHAR* Actions[] = { TEXT("idle"), TEXT("walk"), TEXT("combat"), TEXT("spell"), TEXT("sit"), TEXT("hurt"), TEXT("dying") };
        const TCHAR* Labels[] = { TEXT("Repos"), TEXT("Marche"), TEXT("Poing"), TEXT("Magie"), TEXT("Assis"), TEXT("Blessé"), TEXT("Chute") };
        const float Width = FMath::Min(110.0f, (ViewWidth - 80 - 6 * 8) / 7);
        for (int32 Index = 0; Index < 7; ++Index)
            Button(FName(*(FString(TEXT("preview_")) + Actions[Index])), Labels[Index], 40 + Index * (Width + 8), 574, Width);
    }
    Button(TEXT("previouspage"), TEXT("Précédent"), 40, 642, 170);
    Button(TEXT("nextpage"), TEXT("Suivant"), 226, 642, 170);
    Button(TEXT("closebook"), TEXT("Fermer"), ViewWidth - 210, 642, 170);
}

