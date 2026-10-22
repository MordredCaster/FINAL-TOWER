#include "FTBattleRules.h"
#include <cassert>
#include <climits>
#include <cmath>
#include <iostream>
#include <limits>

using namespace FinalTowerRules;

static Battle Encounter()
{
    Battle Fight;
    Fight.Ally = {40, 20, 8, 2, 10, 2, 7};
    Fight.Enemy = {38, 38, 7, 3, 6, 6, 3};
    return Fight;
}

int main()
{
    const auto Invalid = static_cast<ItemId>(INT_MAX);
    Inventory Bag;
    assert(!FindItem(Invalid) && !FindItem("unknown") && !FindItem(static_cast<const char*>(nullptr)));
    for (const Item& Entry : ItemCatalog()) assert(FindItem(Entry.Key) == &Entry);
    assert(Bag.Quantity(Invalid) == 0 && Bag.Add(Invalid, 1) == 0 && !Bag.Consume(Invalid));
    assert(Bag.Add(ItemId::Berry, -1) == 0 && Bag.Add(ItemId::Berry, 0) == 0);
    assert(Bag.Add(ItemId::Berry, INT_MAX) == 99);
    assert(Bag.Add(ItemId::Berry, INT_MAX) == 0 && Bag.Quantity(ItemId::Berry) == 99);
    for (int i = 0; i < 99; ++i) assert(Bag.Consume(ItemId::Berry));
    assert(!Bag.Consume(ItemId::Berry));

    for (int Version : {1, 2})
    {
        Bag = RestoreInventory(Version, {});
        assert(Bag.Quantity(ItemId::Berry) == 3 && Bag.Quantity(ItemId::Dew) == 2 && Bag.Quantity(ItemId::Treat) == 2);
        Bag.Consume(ItemId::Treat);
        const Inventory Reloaded = RestoreInventory(3, Bag);
        assert(Reloaded.Quantity(ItemId::Treat) == 1);
    }
    assert(RestoreInventory(3, {}).Quantity(ItemId::Berry) == 0);
    Bag.Counts = {-10, INT_MAX, 7};
    Bag = RestoreInventory(3, Bag);
    assert(Bag.Counts[0] == 0 && Bag.Counts[1] == 99 && Bag.Counts[2] == 7);

    Bag = StarterInventory();
    Resources Target{39, 9};
    assert(UseRestorative(Bag, ItemId::Berry, Target, 40, 10));
    assert(Target.HP == 40 && Target.MP == 9 && Bag.Quantity(ItemId::Berry) == 2);
    assert(!UseRestorative(Bag, ItemId::Berry, Target, 40, 10));
    assert(UseRestorative(Bag, ItemId::Dew, Target, 40, 10));
    assert(Target.MP == 10 && Bag.Quantity(ItemId::Dew) == 1);
    Target = {0, 0};
    assert(!UseRestorative(Bag, ItemId::Berry, Target, 40, 10));
    assert(!UseRestorative(Bag, ItemId::Dew, Target, 40, 10));
    assert(!UseRestorative(Bag, ItemId::Treat, Target, 40, 10));
    assert(Bag.Quantity(ItemId::Berry) == 2 && Bag.Quantity(ItemId::Dew) == 1);

    Battle Fight = Encounter();
    assert(Fight.UseItem(Bag, ItemId::Berry));
    assert(Fight.Ally.HP == 35 && Bag.Quantity(ItemId::Berry) == 1); // capped heal, then five damage.
    assert(Fight.UseItem(Bag, ItemId::Dew));
    assert(Fight.Ally.HP == 30 && Fight.Ally.MP == 10 && Bag.Quantity(ItemId::Dew) == 0);
    assert(!Fight.UseItem(Bag, ItemId::Dew) && Fight.Ally.HP == 30);
    assert(!Fight.UseItem(Bag, Invalid) && Fight.Ally.HP == 30);
    Fight.Ally.HP = Fight.Ally.MaxHP;
    assert(!Fight.UseItem(Bag, ItemId::Berry) && Bag.Quantity(ItemId::Berry) == 1);
    Fight.Ally.HP = 1;
    assert(Fight.UseItem(Bag, ItemId::Berry) && Fight.Ally.HP == 21);

    Bag = StarterInventory();
    Fight = Encounter();
    const float Before = Fight.RecruitmentChance();
    assert(Fight.UseItem(Bag, ItemId::Treat));
    assert(std::abs(Fight.RecruitmentChance() - Before - 0.2f) < 0.00001f);
    assert(Fight.Ally.HP == 15 && Bag.Quantity(ItemId::Treat) == 1);
    assert(!Fight.UseItem(Bag, ItemId::Treat) && Fight.Ally.HP == 15);
    assert(Fight.SwitchAlly({30, 30, 6, 1, 12, 12, 9}, Species::Luciolle, 1));
    assert(Fight.Fed && !Fight.CanUseItem(Bag, ItemId::Treat));
    Fight.Enemy.HP = 1;
    assert(Fight.RecruitmentChance() <= 0.95f);
    assert(Fight.TryRecruit(0.94f) && Fight.Result == Outcome::Recruited);
    assert(!Fight.UseItem(Bag, ItemId::Treat));
    Fight = Encounter();
    assert(!Fight.Fed && Fight.CanUseItem(Bag, ItemId::Treat));
    Fight.Recruitable = false;
    assert(!Fight.UseItem(Bag, ItemId::Treat));

    Fight = Encounter();
    Fight.Ally.HP = 1;
    assert(Fight.UseItem(Bag, ItemId::Treat) && Fight.Result == Outcome::Defeat);
    assert(Fight.Fed && Bag.Quantity(ItemId::Treat) == 0);
    assert(!Fight.UseItem(Bag, ItemId::Berry)); // cannot heal or spend objects during forced replacement.
    assert(Fight.SwitchAlly({40, 40, 8, 2, 10, 10, 7}, Species::Mousse, 1) && Fight.Fed);
    Fight.Escape();
    assert(!Fight.UseItem(Bag, ItemId::Berry));

    // Actual training loop: combat rewards replenish supplies, captures never call this grant.
    Bag = {};
    for (Species Enemy : {Species::Mousse, Species::Roncinet, Species::Luciolle})
    {
        const Inventory Loot = VictoryLoot(Enemy);
        for (const Item& Entry : ItemCatalog()) Bag.Add(Entry.Id, Loot.Quantity(Entry.Id));
    }
    assert(Bag.Quantity(ItemId::Berry) == 2 && Bag.Quantity(ItemId::Dew) == 1 && Bag.Quantity(ItemId::Treat) == 1);
    Bag.Add(ItemId::Berry, 98);
    assert(Bag.Quantity(ItemId::Berry) == 99);
    Bag = RestoreInventory(3, Bag);
    assert(Bag.Quantity(ItemId::Berry) == 99 && Bag.Quantity(ItemId::Treat) == 1);
    std::cout << "Inventory, item turns, food, loot and inventory migration rules passed\n";
}
