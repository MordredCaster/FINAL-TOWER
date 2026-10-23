#include "FTPrototype.h"
#include "FTAdventureSubsystem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

namespace
{
UFTAdventureSubsystem* MenuAdventure(const UObject* Context)
{
    UGameInstance* Instance = UGameplayStatics::GetGameInstance(Context);
    return Instance ? Instance->GetSubsystem<UFTAdventureSubsystem>() : nullptr;
}
}

void AFTPrototypeHUD::BeginPlay()
{
    Super::BeginPlay();
    RefreshMainMenuSave();
    MainMenuFocus = bCanContinue ? 0 : 1;
}

void AFTPrototypeHUD::RefreshMainMenuSave()
{
    const UFTAdventureSubsystem* State = MenuAdventure(this);
    bSaveExists = State && State->HasSaveGame();
    bCanContinue = State && State->CanContinueGame();
}

void AFTPrototypeHUD::HandleMainMenuKeys()
{
    if (!PlayerOwner || !IsMainMenuOpen()) return;
    const bool bConfirm = MainMenu.Current() == FinalTowerRules::MainMenu::Screen::ConfirmOverwrite;
    if (PlayerOwner->WasInputKeyJustPressed(EKeys::Escape))
    {
        if (bConfirm) MainMenuAction(TEXT("title_cancel"));
        return;
    }
    if (PlayerOwner->WasInputKeyJustPressed(EKeys::Up) || PlayerOwner->WasInputKeyJustPressed(EKeys::Down) ||
        PlayerOwner->WasInputKeyJustPressed(EKeys::Tab))
        MainMenuFocus = !bConfirm && !bCanContinue ? 1 : 1 - MainMenuFocus;
    if (PlayerOwner->WasInputKeyJustPressed(EKeys::Enter) || PlayerOwner->WasInputKeyJustPressed(EKeys::SpaceBar))
    {
        const FName Action = bConfirm ? (MainMenuFocus == 0 ? TEXT("title_cancel") : TEXT("title_confirm")) :
            (MainMenuFocus == 0 ? TEXT("title_continue") : TEXT("title_new"));
        MainMenuAction(Action);
    }
}

void AFTPrototypeHUD::FinishMainMenuStart(bool bSucceeded)
{
    MainMenu.FinishStart(bSucceeded);
    Buttons.Reset();
    if (!bSucceeded)
    {
        RefreshMainMenuSave();
        MainMenuFocus = bCanContinue ? 0 : 1;
    }
}

void AFTPrototypeHUD::MainMenuAction(FName Action)
{
    UFTAdventureSubsystem* State = MenuAdventure(this);
    if (!State || !IsMainMenuOpen()) return;
    Buttons.Reset();
    if (Action == TEXT("title_cancel"))
    {
        MainMenu.Cancel();
        MainMenuFocus = 1;
        MainMenuMessage.Reset();
    }
    else if (Action == TEXT("title_continue"))
    {
        if (!MainMenu.Continue(bCanContinue)) return;
        const bool bLoaded = State->LoadProgress();
        if (!bLoaded) MainMenuMessage = TEXT("Chargement impossible. La partie n'a pas été ouverte.");
        FinishMainMenuStart(bLoaded);
    }
    else if (Action == TEXT("title_new"))
    {
        // Recheck the disk at the moment of the request, including unreadable saves.
        RefreshMainMenuSave();
        MainMenuMessage.Reset();
        if (MainMenu.NewGame(bSaveExists))
        {
            const bool bStarted = State->StartNewGame(false);
            if (!bStarted) MainMenuMessage = TEXT("Création impossible. Vérifiez l'accès aux sauvegardes.");
            FinishMainMenuStart(bStarted);
        }
        else if (MainMenu.Current() == FinalTowerRules::MainMenu::Screen::ConfirmOverwrite)
            MainMenuFocus = 0; // Cancel is selected by default, never the destructive action.
    }
    else if (Action == TEXT("title_confirm") && MainMenu.Confirm())
    {
        const bool bStarted = State->StartNewGame(true);
        if (!bStarted) MainMenuMessage = TEXT("Création impossible. Vérifiez l'accès aux sauvegardes.");
        FinishMainMenuStart(bStarted);
    }
}

void AFTPrototypeHUD::DrawMainMenu()
{
    // Fit the complete composition on landscape phones, 16:9, 16:10 and 4:3.
    // All hit boxes use the very same transform as the painted controls.
    Scale = FMath::Max(0.01f, FMath::Min(Canvas->SizeX / 1280.0f, Canvas->SizeY / 720.0f));
    const float Left = (Canvas->SizeX / Scale - 1280.0f) * 0.5f;
    const float Top = (Canvas->SizeY / Scale - 720.0f) * 0.5f;
    const FLinearColor Gold(0.82f, 0.65f, 0.36f);
    const FLinearColor Pale(0.94f, 0.91f, 0.82f);
    const FLinearColor Muted(0.55f, 0.64f, 0.68f);
    const auto Rect = [&](float X, float Y, float W, float H, FLinearColor Color)
    {
        DrawRect(Color, (Left + X) * Scale, (Top + Y) * Scale, W * Scale, H * Scale);
    };
    const auto Line = [&](float X, float Y, float X2, float Y2, FLinearColor Color, float Thickness = 1.0f)
    {
        DrawLine((Left + X) * Scale, (Top + Y) * Scale, (Left + X2) * Scale, (Top + Y2) * Scale, Color, Thickness * Scale);
    };
    const auto Text = [&](const FString& Value, float X, float Y, FLinearColor Color, float Size = 1.0f)
    {
        Label(Value, Left + X, Top + Y, Color, Size);
    };
    const auto Frame = [&](float X, float Y, float W, float H)
    {
        Rect(X, Y, W, H, FLinearColor(0.035f, 0.075f, 0.095f));
        Line(X, Y, X + W, Y, Gold);
        Line(X, Y + H, X + W, Y + H, Gold);
        Line(X, Y, X, Y + H, Gold);
        Line(X + W, Y, X + W, Y + H, Gold);
        Line(X + 10, Y + 10, X + 36, Y + 10, Gold, 2);
        Line(X + 10, Y + 10, X + 10, Y + 36, Gold, 2);
        Line(X + W - 36, Y + H - 10, X + W - 10, Y + H - 10, Gold, 2);
        Line(X + W - 10, Y + H - 36, X + W - 10, Y + H - 10, Gold, 2);
    };
    const auto Choice = [&](FName Action, const FString& Caption, float X, float Y, float W, int32 Index, bool bEnabled = true)
    {
        float MouseX = 0, MouseY = 0;
        const bool bMouse = PlayerOwner && PlayerOwner->GetMousePosition(MouseX, MouseY);
        const FVector2D Position((Left + X) * Scale, (Top + Y) * Scale);
        const FVector2D Size(W * Scale, 62 * Scale);
        const bool bHover = bMouse && MouseX >= Position.X && MouseX <= Position.X + Size.X &&
            MouseY >= Position.Y && MouseY <= Position.Y + Size.Y;
        const bool bHighlight = bEnabled && (MainMenuFocus == Index || bHover);
        Rect(X, Y, W, 62, bHighlight ? FLinearColor(0.12f, 0.22f, 0.25f) : FLinearColor(0.055f, 0.11f, 0.14f));
        Line(X, Y + 62, X + W, Y + 62, bHighlight ? Gold : FLinearColor(0.16f, 0.25f, 0.28f));
        if (bHighlight) Rect(X, Y, 3, 62, Gold);
        float TW = 0, TH = 0;
        GetTextSize(Caption, TW, TH, GEngine->GetMediumFont(), Scale * 1.1f);
        Text(Caption, X + (W - TW / Scale) * 0.5f, Y + (62 - TH / Scale) * 0.5f,
            bEnabled ? Pale : Muted * 0.65f, 1.1f);
        if (bEnabled) Buttons.Add({Action, Position, Size});
    };

    for (int32 Band = 0; Band < 24; ++Band)
    {
        const float T = Band / 23.0f;
        DrawRect(FLinearColor(0.015f + T * 0.015f, 0.035f + T * 0.03f, 0.06f + T * 0.035f),
            0, Canvas->SizeY * Band / 24.0f, Canvas->SizeX, Canvas->SizeY / 24.0f + 1);
    }
    Line(40, 40, 1240, 40, FLinearColor(0.22f, 0.27f, 0.28f));
    Line(40, 680, 1240, 680, FLinearColor(0.22f, 0.27f, 0.28f));
    Text(TEXT("UNE AVENTURE, DES RENCONTRES"), 90, 90, Muted, 0.85f);

    // A tower emblem made from Canvas primitives, with no texture dependency.
    const float TowerX = 285;
    Rect(TowerX, 190, 110, 158, FLinearColor(0.06f, 0.13f, 0.17f));
    Line(TowerX - 20, 190, TowerX + 55, 120, Gold, 2);
    Line(TowerX + 55, 120, TowerX + 130, 190, Gold, 2);
    Line(TowerX - 20, 190, TowerX + 130, 190, Gold, 2);
    Line(TowerX, 190, TowerX, 348, Gold, 2);
    Line(TowerX + 110, 190, TowerX + 110, 348, Gold, 2);
    for (int32 Row = 0; Row < 3; ++Row)
    {
        Rect(TowerX + 46, 210 + Row * 43, 18, 25, Gold * (0.85f - Row * 0.15f));
        Line(TowerX - 10, 245 + Row * 43, TowerX + 120, 245 + Row * 43, Gold);
    }
    Line(225, 350, 455, 350, Gold, 2);
    Text(TEXT("FINAL TOWER"), 92, 385, Pale, 2.8f);
    Line(94, 462, 226, 462, Gold, 2);
    Text(TEXT("Chaque rencontre ouvre une histoire."), 94, 485, Muted, 1.05f);
    Text(TEXT("Explorer. Se lier. Grandir."), 94, 522, Gold, 0.95f);

    const bool bConfirm = MainMenu.Current() == FinalTowerRules::MainMenu::Screen::ConfirmOverwrite;
    Frame(720, 150, 460, 422);
    Text(TEXT("VOTRE AVENTURE"), 760, 194, Gold, 1.15f);
    Text(TEXT("Le village vous attend."), 760, 238, Muted);
    if (!bConfirm)
    {
        Choice(TEXT("title_continue"), TEXT("Continuer"), 760, 300, 380, 0, bCanContinue);
        Choice(TEXT("title_new"), TEXT("Nouvelle partie"), 760, 384, 380, 1);
        const FString Status = bCanContinue ? TEXT("Votre sauvegarde est prête.") :
            (bSaveExists ? TEXT("Sauvegarde illisible ou incompatible.") : TEXT("Aucune aventure sauvegardée."));
        Text(Status, 760, 478, Muted, 0.8f);
        Text(TEXT("Une aventure solo, à votre rythme."), 760, 514, Muted, 0.8f);
    }
    if (!MainMenuMessage.IsEmpty())
        Text(MainMenuMessage, 94, 606, FLinearColor(1, 0.67f, 0.47f), 0.85f);
    Text(TEXT("Toucher pour choisir  •  Haut / Bas : choisir  •  Entrée : valider"), 94, 646, Muted, 0.8f);

    if (bConfirm)
    {
        DrawRect(FLinearColor(0, 0, 0, 0.78f), 0, 0, Canvas->SizeX, Canvas->SizeY);
        Frame(290, 180, 700, 360);
        Text(TEXT("RECOMMENCER L'AVENTURE ?"), 336, 218, Gold, 1.25f);
        Text(TEXT("Votre sauvegarde actuelle sera remplacée."), 336, 282, Pale);
        Text(TEXT("Compagnons, objets et quêtes repartiront de zéro."), 336, 320, Muted);
        Text(TEXT("Cette action est définitive."), 336, 358, Muted);
        Choice(TEXT("title_cancel"), TEXT("Annuler"), 336, 424, 276, 0);
        Choice(TEXT("title_confirm"), TEXT("Recommencer"), 636, 424, 308, 1);
    }
}
