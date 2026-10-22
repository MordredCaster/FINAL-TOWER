#include "FTBattleRules.h"
#include <cassert>
#include <climits>
#include <iostream>

using namespace FinalTowerRules;

static Fighter FighterFor(Species Id, int Level)
{
    const auto S = StatsAtLevel(Id, Level);
    return {S.MaxHP, S.MaxHP, S.Attack, S.Defense, S.MaxMP, S.MaxMP, S.Magic};
}

static Battle Encounter(Species Id = Species::Mousse, int Level = 1)
{
    Battle B;
    B.Ally = FighterFor(Id, Level);
    B.Enemy = {200, 200, 6, 1};
    B.AllySpecies = Id;
    B.AllyLevel = Level;
    return B;
}

int main()
{
    assert(XPForLevel(1) == 0 && XPForLevel(2) == 20 && XPForLevel(3) == 60 && XPForLevel(5) == 200);
    for (int L = 1; L <= MaxLevel; ++L)
    {
        assert(LevelFromXP(XPForLevel(L)) == L);
        if (L > 1) assert(LevelFromXP(XPForLevel(L) - 1) == L - 1);
    }
    assert(LevelFromXP(-100) == 1 && LevelFromXP(INT_MAX) == MaxLevel);
    assert(GrantXP(10, -100) == 10 && GrantXP(10, INT_MAX) == XPForLevel(MaxLevel));
    assert(GrantXP(-100, 24) == 24 && GrantXP(INT_MAX, INT_MAX) == XPForLevel(MaxLevel));
    const auto Before = StatsAtLevel(Species::Mousse, 1), After = StatsAtLevel(Species::Mousse, 3);
    auto R = OnLevelGrowth({10, 2}, Before, After);
    assert(R.HP == 20 && R.MP == 6); // Preserve missing resources over multiple levels.
    assert(OnLevelGrowth({0, 0}, Before, After).HP == 0);
    assert(StatsAtLevel(Species::Roncinet, 20).Defense > StatsAtLevel(Species::Luciolle, 20).Defense);
    assert(StatsAtLevel(Species::Luciolle, 20).Magic > StatsAtLevel(Species::Roncinet, 20).Magic);

    for (Species Id : {Species::Mousse, Species::Roncinet, Species::Luciolle})
    {
        const auto Skills = SkillsForSpecies(Id);
        assert(HasLearned(Id, 1, Skills[0]) && !HasLearned(Id, 2, Skills[1]));
        assert(HasLearned(Id, 3, Skills[1]) && !HasLearned(Id, 4, Skills[2]));
        assert(HasLearned(Id, 5, Skills[2]));
        assert(Skills[0] != Skills[1] && Skills[1] != Skills[2]);
    }
    assert(!HasLearned(Species::Mousse, 20, SkillId::Rush));
    assert(!HasLearned(Species::Roncinet, 20, SkillId::Glow));

    // Legacy save migration: retain the former active member even if it was beyond the first three.
    const auto Legacy = NormalizeParty({}, 8, 7);
    assert((Legacy == std::vector<int>{7, 0, 1}));
    assert((NormalizeParty({-1, 2, 2, 99, 1, 0, 3}, 4) == std::vector<int>{2, 1, 0}));
    assert(NormalizeParty({}, 0).empty());
    assert((NormalizeParty({5}, 2) == std::vector<int>{0, 1}));

    auto B = Encounter();
    assert(!B.UseSkill(SkillId::BarkSkin) && B.Ally.MP == 10 && B.Ally.HP == 40);
    assert(!B.UseSkill(SkillId::LeafMend) && B.Ally.MP == 10); // Full-health healing does not waste a turn.
    B.Ally.HP = 20;
    assert(B.UseSkill(SkillId::LeafMend) && B.Ally.HP == 31 && B.Ally.MP == 7);
    B.Ally.MP = 0;
    const int HP = B.Ally.HP;
    assert(!B.UseSkill(SkillId::LeafMend) && B.Ally.HP == HP);
    assert(!B.UseSkill(static_cast<SkillId>(999)));

    B = Encounter(Species::Roncinet);
    assert(B.UseSkill(SkillId::Rush) && B.Enemy.HP == 189 && B.Ally.HP == 35 && B.Ally.MP == 4);
    B = Encounter(Species::Luciolle);
    B.Enemy.Defense = 6;
    assert(B.UseSkill(SkillId::Spark) && B.Enemy.HP == 190 && B.Ally.MP == 9);
    B.Enemy.HP = 1;
    const int LivingHP = B.Ally.HP;
    assert(B.UseSkill(SkillId::Spark) && B.Result == Outcome::Victory && B.Ally.HP == LivingHP);
    assert(!B.UseSkill(SkillId::Spark) && !B.TryRecruit(0));

    B = Encounter(Species::Mousse, 3);
    B.Enemy.Attack = 12;
    assert(B.UseSkill(SkillId::BarkSkin) && B.Ally.HP == 49 && B.GuardTurns == 2);
    assert(B.Attack() && B.Ally.HP == 48 && B.GuardTurns == 1);
    assert(B.Attack() && B.Ally.HP == 47 && B.GuardTurns == 0);
    assert(B.Attack() && B.Ally.HP == 41); // Protection expires after exactly three ripostes.

    B = Encounter(Species::Luciolle, 3);
    B.Enemy.Attack = 12;
    assert(B.UseSkill(SkillId::Dazzle) && B.Ally.HP == 33 && B.WeakenTurns == 2);
    B.Attack(); B.Attack();
    assert(B.Ally.HP == 23 && B.WeakenTurns == 0);
    B.Attack(); assert(B.Ally.HP == 14);

    // Voluntary relay pays an enemy turn, preserves the target's MP, and removes personal buffs.
    B = Encounter(Species::Mousse, 3);
    B.UseSkill(SkillId::BarkSkin);
    auto Replacement = FighterFor(Species::Roncinet, 1);
    Replacement.MP = 2;
    assert(B.SwitchAlly(Replacement, Species::Roncinet, 1));
    assert(B.Ally.HP == 35 && B.Ally.MP == 2 && B.GuardTurns == 0);
    assert(B.AllySpecies == Species::Roncinet && !B.CanUseSkill(SkillId::BarkSkin));
    B.Ally.HP = 1;
    B.Attack(); assert(B.Result == Outcome::Defeat);
    assert(!B.Attack() && !B.UseSkill(SkillId::Rush) && !B.Escape());
    assert(B.SwitchAlly(FighterFor(Species::Luciolle, 1), Species::Luciolle, 1));
    assert(B.Ally.HP == 30 && B.Ally.MP == 12 && B.Result == Outcome::Ongoing); // Free replacement after KO.
    auto KO = FighterFor(Species::Mousse, 1); KO.HP = 0;
    assert(!B.SwitchAlly(KO, Species::Mousse, 1));
    B.Enemy.HP = 1; B.Attack();
    assert(!B.SwitchAlly(FighterFor(Species::Mousse, 1), Species::Mousse, 1));

    // Play a complete training progression with the real combat rules and learned techniques.
    int XP = 0;
    for (int Fight = 0; Fight < 8 && LevelFromXP(XP) < 5; ++Fight)
    {
        const int Level = LevelFromXP(XP);
        const int EnemyLevel = Clamp(Level, 1, 3);
        B = Encounter(Species::Mousse, Level);
        B.Enemy = FighterFor(Species::Roncinet, EnemyLevel);
        for (int Turn = 0; B.CanAct() && Turn < 100; ++Turn)
        {
            if (B.Ally.HP < B.Ally.MaxHP / 2 && B.CanUseSkill(SkillId::LeafMend)) B.UseSkill(SkillId::LeafMend);
            else B.Attack();
        }
        assert(B.Result == Outcome::Victory);
        XP = GrantXP(XP, EncounterReward(Species::Roncinet, EnemyLevel));
    }
    assert(LevelFromXP(XP) >= 5 && HasLearned(Species::Mousse, LevelFromXP(XP), SkillId::NaturePulse));
    std::cout << "Progression, skills, relays, migration and full training scenario: all checks passed\n";
}
