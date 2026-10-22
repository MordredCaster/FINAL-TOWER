#include "Modules/ModuleManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "AutomatedAssetImportData.h"
#include "Dom/JsonObject.h"
#include "Engine/Texture2D.h"
#include "Factories/TextureFactory.h"
#include "HAL/FileManager.h"
#include "IAssetTools.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "PaperFlipbook.h"
#include "PaperSprite.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "SpriteEditorOnlyTypes.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY_STATIC(LogFinalTowerArt, Log, All);

namespace
{
const TCHAR* Directions[] = { TEXT("south"), TEXT("west"), TEXT("east"), TEXT("north") };

bool SaveAsset(UObject* Asset)
{
    if (!Asset) return false;
    UPackage* Package = Asset->GetOutermost();
    Package->MarkPackageDirty();
    const FString File = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(File), true);
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    Args.SaveFlags = SAVE_NoError;
    return UPackage::SavePackage(Package, Asset, *File, Args);
}

template<typename T>
T* GetOrCreate(const FString& Folder, const FString& Name)
{
    const FString PackageName = Folder + TEXT("/") + Name;
    if (T* Existing = LoadObject<T>(nullptr, *(PackageName + TEXT(".") + Name))) return Existing;
    UPackage* Package = CreatePackage(*PackageName);
    T* Object = NewObject<T>(Package, *Name, RF_Public | RF_Standalone);
    FAssetRegistryModule::AssetCreated(Object);
    return Object;
}

UTexture2D* ImportTexture(const TSharedPtr<FJsonObject>& Entry, bool bSprite)
{
    const FString Relative = Entry->GetStringField(bSprite ? TEXT("atlas") : TEXT("source"));
    const FString Source = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / Relative);
    if (!FPaths::FileExists(Source))
    {
        UE_LOG(LogFinalTowerArt, Error, TEXT("Missing art source: %s"), *Source);
        return nullptr;
    }
    const FString RequiredName = Entry->GetStringField(TEXT("texture_name"));
    const FString Input = FPaths::ProjectSavedDir() / TEXT("FinalTowerArt/Inputs") / (RequiredName + TEXT(".png"));
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Input), true);
    if (IFileManager::Get().Copy(*Input, *Source, true, true) != COPY_OK) return nullptr;
    // Use a texture factory explicitly: Interchange auto-detection is unnecessary for PNGs.
    UAutomatedAssetImportData* Data = NewObject<UAutomatedAssetImportData>();
    Data->Filenames.Add(FPaths::ConvertRelativePathToFull(Input));
    Data->DestinationPath = Entry->GetStringField(TEXT("destination"));
    Data->bReplaceExisting = true;
    Data->bSkipReadOnly = true;
    Data->Factory = NewObject<UTextureFactory>();
    const TArray<UObject*> Imported = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"))
        .Get().ImportAssetsAutomated(Data);
    UTexture2D* Texture = Imported.IsEmpty() ? nullptr : Cast<UTexture2D>(Imported[0]);
    if (!Texture) return nullptr;
    if (Texture->GetName() != RequiredName) return nullptr;
    Texture->Filter = bSprite ? TF_Nearest : TF_Bilinear;
    Texture->MipGenSettings = TMGS_NoMipmaps;
    Texture->CompressionSettings = TC_EditorIcon;
    Texture->LODGroup = bSprite ? TEXTUREGROUP_Pixels2D : TEXTUREGROUP_UI;
    Texture->SRGB = true;
    Texture->NeverStream = true;
    Texture->PostEditChange();
    return SaveAsset(Texture) ? Texture : nullptr;
}

bool BuildClip(const TSharedPtr<FJsonObject>& Entry)
{
    UTexture2D* Texture = ImportTexture(Entry, true);
    if (!Texture) return false;
    const int32 Cell = Entry->GetIntegerField(TEXT("cell"));
    if (Texture->GetSizeX() != Cell * 4 || Texture->GetSizeY() != Cell * 4) return false;
    const FString Folder = Entry->GetStringField(TEXT("destination"));
    const FString Prefix = Entry->GetStringField(TEXT("actor")) + TEXT("_") + Entry->GetStringField(TEXT("action"));
    for (int32 Row = 0; Row < 4; ++Row)
    {
        TArray<FPaperFlipbookKeyFrame> Frames;
        for (int32 Column = 0; Column < 4; ++Column)
        {
            const FString Name = FString::Printf(TEXT("S_%s_%s_%d"), *Prefix, Directions[Row], Column);
            UPaperSprite* Sprite = GetOrCreate<UPaperSprite>(Folder, Name);
            FSpriteAssetInitParameters Init;
            Init.Texture = Texture;
            Init.Offset = FIntPoint(Column * Cell, Row * Cell);
            Init.Dimension = FIntPoint(Cell, Cell);
            Init.SetPixelsPerUnrealUnit(Entry->GetNumberField(TEXT("pixels_per_unit")));
            Sprite->InitializeSprite(Init, true);
            Sprite->SetPivotMode(ESpritePivotMode::Custom,
                FVector2D(Init.Offset.X + Entry->GetIntegerField(TEXT("pivot_x")),
                          Init.Offset.Y + Entry->GetIntegerField(TEXT("pivot_y"))), true);
            if (!SaveAsset(Sprite)) return false;
            FPaperFlipbookKeyFrame Key;
            Key.Sprite = Sprite;
            Key.FrameRun = 1;
            Frames.Add(Key);
        }
        UPaperFlipbook* Flipbook = GetOrCreate<UPaperFlipbook>(Folder,
            FString::Printf(TEXT("FB_%s_%s"), *Prefix, Directions[Row]));
        {
            FScopedFlipbookMutator Edit(Flipbook);
            Edit.FramesPerSecond = Entry->GetNumberField(TEXT("fps"));
            Edit.KeyFrames = Frames;
        }
        if (!SaveAsset(Flipbook)) return false;
    }
    return true;
}

bool AllPackagesExist(const TSharedPtr<FJsonObject>& Manifest)
{
    for (const TSharedPtr<FJsonValue>& Value : Manifest->GetArrayField(TEXT("clips")))
    {
        const auto E = Value->AsObject();
        const FString Folder = E->GetStringField(TEXT("destination"));
        const FString Prefix = E->GetStringField(TEXT("actor")) + TEXT("_") + E->GetStringField(TEXT("action"));
        if (!FPackageName::DoesPackageExist(Folder / E->GetStringField(TEXT("texture_name")))) return false;
        for (const TCHAR* Direction : Directions)
        {
            if (!FPackageName::DoesPackageExist(Folder / FString::Printf(TEXT("FB_%s_%s"), *Prefix, Direction))) return false;
            for (int32 Frame = 0; Frame < 4; ++Frame)
                if (!FPackageName::DoesPackageExist(Folder / FString::Printf(TEXT("S_%s_%s_%d"), *Prefix, Direction, Frame))) return false;
        }
    }
    for (const TSharedPtr<FJsonValue>& Value : Manifest->GetArrayField(TEXT("concepts")))
    {
        const auto E = Value->AsObject();
        if (!FPackageName::DoesPackageExist(E->GetStringField(TEXT("destination")) / E->GetStringField(TEXT("texture_name")))) return false;
    }
    return true;
}
}

class FFinalTowerEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        // PostEngineInit runs before the first Play session and before commandlet cooking.
        // All output is confined to GeneratedArt; authored assets are never overwritten.
        FString Json;
        const FString File = FPaths::ProjectDir() / TEXT("ArtSource/manifest.json");
        if (!FFileHelper::LoadFileToString(Json, *File))
        {
            UE_LOG(LogFinalTowerArt, Error, TEXT("Missing ArtSource/manifest.json; sprites cannot be built."));
            return;
        }
        TSharedPtr<FJsonObject> Manifest;
        if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Manifest) || !Manifest.IsValid() ||
            Manifest->GetIntegerField(TEXT("schema")) != 1) return;
        const FString Fingerprint = FMD5::HashAnsiString(*Json);
        const FString StampFile = FPaths::ProjectSavedDir() / TEXT("FinalTowerArt/imported.txt");
        FString Previous;
        FFileHelper::LoadFileToString(Previous, *StampFile);
        if (Previous == Fingerprint && AllPackagesExist(Manifest)) return;
        UE_LOG(LogFinalTowerArt, Display, TEXT("Building FINAL TOWER textures, sprites and flipbooks..."));
        for (const auto& Value : Manifest->GetArrayField(TEXT("clips")))
            if (!BuildClip(Value->AsObject()))
            {
                UE_LOG(LogFinalTowerArt, Error, TEXT("Sprite import failed; no completion stamp written."));
                return;
            }
        for (const auto& Value : Manifest->GetArrayField(TEXT("concepts")))
            if (!ImportTexture(Value->AsObject(), false)) return;
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(StampFile), true);
        FFileHelper::SaveStringToFile(Fingerprint, *StampFile);
        UE_LOG(LogFinalTowerArt, Display, TEXT("FINAL TOWER art ready: 160 sprites, 40 flipbooks, 15 textures."));
    }
};

IMPLEMENT_MODULE(FFinalTowerEditorModule, FinalTowerEditor)
