#pragma once
#include "FTInventoryRules.h"

namespace FinalTowerRules
{
enum class DiscoveryId { VillageChest, ForestChest, BerryBush, MoonFlower, OldNest };
enum class DiscoveryKind { Chest, Plant, Clue };
constexpr int DiscoveryCount = 5;

struct Discovery
{
    DiscoveryId Id;
    const char* Key;
    const char* Name;
    DiscoveryKind Kind;
    float X, Y, Z;
    std::array<int, ItemCount> Rewards;
};

inline const std::array<Discovery, DiscoveryCount>& DiscoveryCatalog()
{
    static const std::array<Discovery, DiscoveryCount> Entries = {{
        {DiscoveryId::VillageChest, "village_chest", "Coffre du village", DiscoveryKind::Chest, 80, -360, 20, {2, 0, 1}},
        {DiscoveryId::ForestChest, "forest_chest", "Coffre sous les pins", DiscoveryKind::Chest, -530, 340, 20, {0, 1, 2}},
        {DiscoveryId::BerryBush, "berry_bush", "Buisson de braise", DiscoveryKind::Plant, -430, 80, 20, {2, 0, 0}},
        {DiscoveryId::MoonFlower, "moon_flower", "Fleurs de lune", DiscoveryKind::Plant, 120, 470, 20, {0, 2, 0}},
        {DiscoveryId::OldNest, "old_nest", "Nid sous les branches", DiscoveryKind::Clue, -280, 500, 12, {0, 0, 0}}
    }};
    return Entries;
}

inline const Discovery* FindDiscovery(DiscoveryId Id)
{
    for (const auto& Entry : DiscoveryCatalog()) if (Entry.Id == Id) return &Entry;
    return nullptr;
}

inline const Discovery* FindDiscovery(const char* Key)
{
    if (Key) for (const auto& Entry : DiscoveryCatalog()) if (std::strcmp(Key, Entry.Key) == 0) return &Entry;
    return nullptr;
}

enum class WoundedStep { Hidden, Found, Healed, Trusted, Completed };

struct Exploration
{
    std::array<bool, DiscoveryCount> Collected{};
    WoundedStep Wounded = WoundedStep::Hidden;

    bool IsCollected(DiscoveryId Id) const
    {
        return FindDiscovery(Id) && Collected[static_cast<int>(Id)];
    }

    bool ReadyToReturn() const
    {
        return Wounded == WoundedStep::Trusted && IsCollected(DiscoveryId::OldNest);
    }

    int DiscoveredCount() const
    {
        int Count = 0;
        for (bool Found : Collected) if (Found) ++Count;
        return Count;
    }
};

inline Inventory DiscoveryLoot(const Discovery& Entry)
{
    Inventory Loot;
    for (const Item& Object : ItemCatalog()) Loot.Add(Object.Id, Entry.Rewards[static_cast<int>(Object.Id)]);
    return Loot;
}

inline bool FitsLoot(const Inventory& Bag, const Inventory& Loot)
{
    for (const Item& Entry : ItemCatalog())
        if (Loot.Quantity(Entry.Id) > MaxItemStack - Bag.Quantity(Entry.Id)) return false;
    return true;
}

inline void MergeLoot(Inventory& Bag, const Inventory& Loot)
{
    for (const Item& Entry : ItemCatalog()) Bag.Add(Entry.Id, Loot.Quantity(Entry.Id));
}

enum class CollectResult { Collected, AlreadyCollected, BagFull, Unknown };

inline CollectResult CollectDiscovery(Exploration& World, Inventory& Bag, DiscoveryId Id)
{
    const Discovery* Entry = FindDiscovery(Id);
    if (!Entry) return CollectResult::Unknown;
    if (World.IsCollected(Id)) return CollectResult::AlreadyCollected;
    const Inventory Loot = DiscoveryLoot(*Entry);
    // No partial reward or lost discovery if one of the stacks has insufficient room.
    if (!FitsLoot(Bag, Loot)) return CollectResult::BagFull;
    MergeLoot(Bag, Loot);
    World.Collected[static_cast<int>(Id)] = true;
    return CollectResult::Collected;
}

// Each interaction advances one step. Finding the nest is independent of this order.
inline bool HelpWounded(Exploration& World, Inventory& Bag)
{
    switch (World.Wounded)
    {
    case WoundedStep::Hidden: World.Wounded = WoundedStep::Found; return true;
    case WoundedStep::Found:
        if (!Bag.Consume(ItemId::Berry)) return false;
        World.Wounded = WoundedStep::Healed; return true;
    case WoundedStep::Healed:
        if (!Bag.Consume(ItemId::Treat)) return false;
        World.Wounded = WoundedStep::Trusted; return true;
    default: return false;
    }
}

inline Inventory WoundedReward()
{
    Inventory Reward;
    Reward.Add(ItemId::Dew, 2);
    return Reward;
}

inline bool ClaimWoundedReward(Exploration& World, Inventory& Bag, int RosterCount)
{
    const Inventory Reward = WoundedReward();
    if (!World.ReadyToReturn() || RosterCount < 1 || RosterCount >= 1000 || !FitsLoot(Bag, Reward)) return false;
    MergeLoot(Bag, Reward);
    World.Wounded = WoundedStep::Completed;
    return true; // Adapter adds one level-2 Mousse as part of the same saved transaction.
}

inline Exploration RestoreExploration(int Version, const Exploration& Saved)
{
    if (Version < 4) return {};
    Exploration Result = Saved;
    Result.Wounded = static_cast<WoundedStep>(Clamp(static_cast<int>(Saved.Wounded), 0, 4));
    // A completed quest must stay claimed even if its clue flag was missing.
    if (Result.Wounded == WoundedStep::Completed) Result.Collected[static_cast<int>(DiscoveryId::OldNest)] = true;
    return Result;
}

inline const char* WoundedObjective(const Exploration& World)
{
    switch (World.Wounded)
    {
    case WoundedStep::Hidden: return "Explorer l'ouest du village : une créature se cache près des arbres.";
    case WoundedStep::Found: return "Apporter une baie de braise à la Mousse blessée, à l'ouest du village.";
    case WoundedStep::Healed: return "Offrir une friandise des bois à la Mousse pour gagner sa confiance.";
    case WoundedStep::Trusted: return World.ReadyToReturn() ? "Retourner voir la gardienne avec la Mousse et le ruban du nid." :
        "Fouiller le nid sous les branches, au nord-ouest. Mousse attend votre retour.";
    case WoundedStep::Completed: return "Mousse a retrouvé un foyer. Elle vous accompagne désormais.";
    }
    return "Explorer les environs du village.";
}
}
