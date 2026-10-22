#include "FTExplorationRules.h"
#include <cassert>
#include <climits>
#include <iostream>

using namespace FinalTowerRules;

static void Reload(Exploration& World, Inventory& Bag)
{
    World = RestoreExploration(4, World);
    Bag = RestoreInventory(4, Bag);
}

int main()
{
    const auto Unknown = static_cast<DiscoveryId>(INT_MAX);
    assert(!FindDiscovery(Unknown) && !FindDiscovery("missing") && !FindDiscovery(static_cast<const char*>(nullptr)));
    Exploration World;
    Inventory Bag;
    assert(!World.IsCollected(Unknown));
    assert(CollectDiscovery(World, Bag, Unknown) == CollectResult::Unknown);
    assert(World.DiscoveredCount() == 0 && !World.ReadyToReturn());
    for (const auto& Entry : DiscoveryCatalog())
    {
        assert(FindDiscovery(Entry.Key) == &Entry);
        // Discoveries must stay inside the playable bounds and outside the solid house (+pawn margin).
        assert(Entry.X > -650 && Entry.X < 650 && Entry.Y > -650 && Entry.Y < 650);
        assert(!(Entry.X > -298 && Entry.X < -2 && Entry.Y > -508 && Entry.Y < -292));
    }

    // Loot must be atomic: don't lose a plant/chest or grant partial loot when one stack is full.
    Bag.Add(ItemId::Berry, 98);
    assert(CollectDiscovery(World, Bag, DiscoveryId::VillageChest) == CollectResult::BagFull);
    assert(Bag.Quantity(ItemId::Berry) == 98 && Bag.Quantity(ItemId::Treat) == 0);
    assert(!World.IsCollected(DiscoveryId::VillageChest));
    Bag.Consume(ItemId::Berry);
    assert(CollectDiscovery(World, Bag, DiscoveryId::VillageChest) == CollectResult::Collected);
    assert(Bag.Quantity(ItemId::Berry) == 99 && Bag.Quantity(ItemId::Treat) == 1);
    Reload(World, Bag);
    assert(CollectDiscovery(World, Bag, DiscoveryId::VillageChest) == CollectResult::AlreadyCollected);
    assert(Bag.Quantity(ItemId::Berry) == 99 && Bag.Quantity(ItemId::Treat) == 1);
    assert(CollectDiscovery(World, Bag, DiscoveryId::OldNest) == CollectResult::Collected); // a clue fits a full bag.
    assert(World.DiscoveredCount() == 2);

    // Meeting a creature doesn't silently spend a berry; healing and trust each require their own object.
    World = {}; Bag = {};
    assert(!ClaimWoundedReward(World, Bag, 1));
    assert(HelpWounded(World, Bag) && World.Wounded == WoundedStep::Found);
    Reload(World, Bag);
    assert(!HelpWounded(World, Bag) && World.Wounded == WoundedStep::Found);
    assert(CollectDiscovery(World, Bag, DiscoveryId::BerryBush) == CollectResult::Collected);
    assert(HelpWounded(World, Bag) && Bag.Quantity(ItemId::Berry) == 1);
    assert(World.Wounded == WoundedStep::Healed);
    Reload(World, Bag);
    assert(!HelpWounded(World, Bag));
    assert(CollectDiscovery(World, Bag, DiscoveryId::ForestChest) == CollectResult::Collected);
    assert(HelpWounded(World, Bag) && Bag.Quantity(ItemId::Treat) == 1);
    assert(World.Wounded == WoundedStep::Trusted && !World.ReadyToReturn());
    assert(!HelpWounded(World, Bag) && !ClaimWoundedReward(World, Bag, 1));
    assert(CollectDiscovery(World, Bag, DiscoveryId::OldNest) == CollectResult::Collected);
    Reload(World, Bag);
    assert(World.ReadyToReturn());
    const int DewBefore = Bag.Quantity(ItemId::Dew);
    assert(!ClaimWoundedReward(World, Bag, 1000));
    assert(!ClaimWoundedReward(World, Bag, 0) && World.ReadyToReturn());
    Bag.Add(ItemId::Dew, 98);
    assert(!ClaimWoundedReward(World, Bag, 1) && Bag.Quantity(ItemId::Dew) == 99);
    Bag.Consume(ItemId::Dew); Bag.Consume(ItemId::Dew);
    assert(ClaimWoundedReward(World, Bag, 1));
    assert(World.Wounded == WoundedStep::Completed && Bag.Quantity(ItemId::Dew) == 99);
    assert(DewBefore == 1);
    Reload(World, Bag);
    assert(!ClaimWoundedReward(World, Bag, 2) && !HelpWounded(World, Bag));
    assert(Bag.Quantity(ItemId::Dew) == 99);

    // Nonlinear exploration: finding the clue first still allows the entire story and exactly one reward.
    World = {}; Bag = StarterInventory();
    assert(CollectDiscovery(World, Bag, DiscoveryId::OldNest) == CollectResult::Collected);
    assert(HelpWounded(World, Bag)); Reload(World, Bag);
    assert(HelpWounded(World, Bag)); Reload(World, Bag);
    assert(HelpWounded(World, Bag)); Reload(World, Bag);
    assert(World.ReadyToReturn());
    int AddedCompanions = 0;
    if (ClaimWoundedReward(World, Bag, 4)) ++AddedCompanions;
    Reload(World, Bag);
    if (ClaimWoundedReward(World, Bag, 5)) ++AddedCompanions;
    assert(AddedCompanions == 1 && Bag.Quantity(ItemId::Dew) == 4);
    assert(Bag.Quantity(ItemId::Berry) == 2 && Bag.Quantity(ItemId::Treat) == 1);

    // Every old version starts this new story untouched, while its inventory follows the V3 migration rule.
    for (int Version : {1, 2, 3})
    {
        const auto Migrated = RestoreExploration(Version, World);
        assert(Migrated.DiscoveredCount() == 0 && Migrated.Wounded == WoundedStep::Hidden);
        const auto OldBag = RestoreInventory(Version, Bag);
        assert(OldBag.Quantity(ItemId::Berry) == (Version < 3 ? 3 : 2));
    }
    World.Collected = {};
    World = RestoreExploration(4, World);
    assert(World.Wounded == WoundedStep::Completed && World.IsCollected(DiscoveryId::OldNest));
    assert(!ClaimWoundedReward(World, Bag, 2)); // an incomplete clue flag cannot make the reward repeat.
    World.Wounded = static_cast<WoundedStep>(-42);
    assert(RestoreExploration(4, World).Wounded == WoundedStep::Hidden);

    // All five finite discoveries persist through reloads and can only produce one set of supplies.
    World = {}; Bag = {};
    for (const auto& Entry : DiscoveryCatalog())
    {
        assert(CollectDiscovery(World, Bag, Entry.Id) == CollectResult::Collected);
        Reload(World, Bag);
    }
    assert(World.DiscoveredCount() == 5);
    assert(Bag.Quantity(ItemId::Berry) == 4 && Bag.Quantity(ItemId::Dew) == 3 && Bag.Quantity(ItemId::Treat) == 3);
    for (const auto& Entry : DiscoveryCatalog()) assert(CollectDiscovery(World, Bag, Entry.Id) == CollectResult::AlreadyCollected);
    std::cout << "Exploration: atomic loot, finite discoveries, full quest, reordered clues and migration passed\n";
}
