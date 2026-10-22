#pragma once
#include "FTProgressionRules.h"
#include <array>
#include <cstring>

namespace FinalTowerRules
{
enum class ItemId { Berry, Dew, Treat };
enum class ItemEffect { HealHP, RestoreMP, Befriend };
constexpr int ItemCount = 3;
constexpr int MaxItemStack = 99;

struct Item
{
    ItemId Id;
    const char* Key; // Stable save identifier; display names can change.
    const char* Name;
    const char* Description;
    ItemEffect Effect;
    int Power;
};

inline const std::array<Item, ItemCount>& ItemCatalog()
{
    static const std::array<Item, ItemCount> Entries = {{
        {ItemId::Berry, "healing_berry", "Baie de braise", "Rend 25 PV à un compagnon debout.", ItemEffect::HealHP, 25},
        {ItemId::Dew, "moon_dew", "Rosée de lune", "Rend 8 PM à un compagnon debout.", ItemEffect::RestoreMP, 8},
        {ItemId::Treat, "friendship_treat", "Friandise des bois", "+20 points d'amitié pour ce combat. Une seule par rencontre.", ItemEffect::Befriend, 20}
    }};
    return Entries;
}

inline const Item* FindItem(ItemId Id)
{
    for (const Item& Entry : ItemCatalog()) if (Entry.Id == Id) return &Entry;
    return nullptr;
}

inline const Item* FindItem(const char* Key)
{
    if (Key) for (const Item& Entry : ItemCatalog()) if (std::strcmp(Entry.Key, Key) == 0) return &Entry;
    return nullptr;
}

struct Inventory
{
    std::array<int, ItemCount> Counts{};

    int Quantity(ItemId Id) const
    {
        const Item* Entry = FindItem(Id);
        return Entry ? Clamp(Counts[static_cast<int>(Entry->Id)], 0, MaxItemStack) : 0;
    }

    int Add(ItemId Id, int Amount)
    {
        if (!FindItem(Id) || Amount <= 0) return 0;
        const int Before = Quantity(Id);
        const int Added = Clamp(Amount, 0, MaxItemStack - Before);
        Counts[static_cast<int>(Id)] = Before + Added;
        return Added;
    }

    bool Consume(ItemId Id)
    {
        if (Quantity(Id) <= 0) return false;
        Counts[static_cast<int>(Id)] = Quantity(Id) - 1;
        return true;
    }
};

inline Inventory StarterInventory()
{
    Inventory Bag;
    Bag.Add(ItemId::Berry, 3); Bag.Add(ItemId::Dew, 2); Bag.Add(ItemId::Treat, 2);
    return Bag;
}

// V1/V2 had no inventory. V3's empty bag must stay empty when loaded.
inline Inventory RestoreInventory(int Version, const Inventory& Saved)
{
    if (Version < 3) return StarterInventory();
    Inventory Result;
    for (const Item& Entry : ItemCatalog()) Result.Add(Entry.Id, Saved.Quantity(Entry.Id));
    return Result;
}

inline bool CanRestore(const Item& Entry, Resources Target, int MaxHP, int MaxMP)
{
    if (Target.HP <= 0) return false; // These objects never revive a knocked-out monster.
    return (Entry.Effect == ItemEffect::HealHP && Target.HP < MaxHP) ||
        (Entry.Effect == ItemEffect::RestoreMP && Target.MP < MaxMP);
}

inline bool UseRestorative(Inventory& Bag, ItemId Id, Resources& Target, int MaxHP, int MaxMP)
{
    const Item* Entry = FindItem(Id);
    if (!Entry || Bag.Quantity(Id) == 0 || !CanRestore(*Entry, Target, MaxHP, MaxMP)) return false;
    if (Entry->Effect == ItemEffect::HealHP) Target.HP += Clamp(Entry->Power, 0, MaxHP - Target.HP);
    else Target.MP += Clamp(Entry->Power, 0, MaxMP - Target.MP);
    Bag.Consume(Id);
    return true;
}

// Ordinary victories supply the next encounter. Captures and escapes give no items.
inline Inventory VictoryLoot(Species Enemy)
{
    Inventory Loot;
    if (Enemy == Species::Luciolle) Loot.Add(ItemId::Dew, 1);
    else
    {
        Loot.Add(ItemId::Berry, 1);
        if (Enemy == Species::Roncinet) Loot.Add(ItemId::Treat, 1);
    }
    return Loot;
}
}
