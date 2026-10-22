#pragma once

#include <array>
#include <vector>

namespace FinalTowerRules
{
constexpr int MaxLevel = 20;
constexpr int PartyCapacity = 3;
enum class Species { Mousse, Luciolle, Roncinet };
enum class SkillId { LeafMend, BarkSkin, NaturePulse, Rush, Brace, EarthBreak, Spark, Dazzle, Glow };
enum class SkillEffect { Physical, Magical, Heal, Guard, Weaken };

inline int Clamp(int Value, int Low, int High) { return Value < Low ? Low : (Value > High ? High : Value); }
inline int XPForLevel(int Level)
{
    Level = Clamp(Level, 1, MaxLevel);
    return 10 * Level * (Level - 1);
}
inline int LevelFromXP(int Experience)
{
    int Level = 1;
    while (Level < MaxLevel && Experience >= XPForLevel(Level + 1)) ++Level;
    return Level;
}
inline int GrantXP(int Experience, int Reward)
{
    Experience = Clamp(Experience, 0, XPForLevel(MaxLevel));
    const int Room = XPForLevel(MaxLevel) - Experience;
    return Experience + Clamp(Reward, 0, Room); // No overflow, even for an invalid huge reward.
}

struct MonsterStats { int MaxHP; int Attack; int Defense; int MaxMP; int Magic; };
inline MonsterStats StatsAtLevel(Species Id, int Level)
{
    const int Growth = Clamp(Level, 1, MaxLevel) - 1;
    switch (Id)
    {
    case Species::Roncinet: return {38 + 6*Growth, 7 + 3*Growth, 3 + 2*Growth, 6 + Growth, 3 + Growth};
    case Species::Luciolle: return {30 + 4*Growth, 6 + Growth, 1 + Growth, 12 + 3*Growth, 9 + 3*Growth};
    default: return {40 + 5*Growth, 8 + 2*Growth, 2 + 2*Growth, 10 + 2*Growth, 7 + 2*Growth};
    }
}

struct Resources { int HP; int MP; };
inline Resources OnLevelGrowth(Resources Current, MonsterStats Before, MonsterStats After)
{
    // Growing max HP never revives a knocked-out monster or completely refills resources.
    return {Current.HP <= 0 ? 0 : Clamp(Current.HP + After.MaxHP - Before.MaxHP, 1, After.MaxHP),
            Clamp(Current.MP + After.MaxMP - Before.MaxMP, 0, After.MaxMP)};
}

struct Skill
{
    SkillId Id;
    const char* Name;
    const char* Description;
    int UnlockLevel;
    int MPCost;
    SkillEffect Effect;
    int Power;
    int Duration;
};

inline const std::array<Skill, 9>& SkillCatalog()
{
    static const std::array<Skill, 9> Catalog = {{
        {SkillId::LeafMend, "Pousse vitale", "Rend des PV au compagnon actif.", 1, 3, SkillEffect::Heal, 8, 0},
        {SkillId::BarkSkin, "Écorce", "Défense +5 contre trois ripostes.", 3, 3, SkillEffect::Guard, 5, 3},
        {SkillId::NaturePulse, "Souffle vert", "Dégâts magiques de nature.", 5, 5, SkillEffect::Magical, 8, 0},
        {SkillId::Rush, "Charge", "Une attaque physique renforcée.", 1, 2, SkillEffect::Physical, 5, 0},
        {SkillId::Brace, "Enracinement", "Défense +7 contre deux ripostes.", 3, 3, SkillEffect::Guard, 7, 2},
        {SkillId::EarthBreak, "Brise-roche", "Une puissante attaque physique.", 5, 5, SkillEffect::Physical, 12, 0},
        {SkillId::Spark, "Étincelle", "Dégâts magiques lumineux.", 1, 3, SkillEffect::Magical, 4, 0},
        {SkillId::Dazzle, "Éblouissement", "Attaque ennemie -4 pendant trois ripostes.", 3, 3, SkillEffect::Weaken, 4, 3},
        {SkillId::Glow, "Lueur apaisante", "Rend beaucoup de PV au compagnon actif.", 5, 4, SkillEffect::Heal, 12, 0}
    }};
    return Catalog;
}

inline std::array<SkillId, 3> SkillsForSpecies(Species Id)
{
    switch (Id)
    {
    case Species::Roncinet: return {SkillId::Rush, SkillId::Brace, SkillId::EarthBreak};
    case Species::Luciolle: return {SkillId::Spark, SkillId::Dazzle, SkillId::Glow};
    default: return {SkillId::LeafMend, SkillId::BarkSkin, SkillId::NaturePulse};
    }
}
inline const Skill* FindSkill(SkillId Id)
{
    for (const Skill& Entry : SkillCatalog()) if (Entry.Id == Id) return &Entry;
    return nullptr;
}
inline bool HasLearned(Species Id, int Level, SkillId Technique)
{
    const Skill* Entry = FindSkill(Technique);
    if (!Entry || Level < Entry->UnlockLevel) return false;
    for (SkillId Known : SkillsForSpecies(Id)) if (Known == Technique) return true;
    return false;
}
inline int EncounterReward(Species Id, int Level)
{
    const int Base = Id == Species::Roncinet ? 30 : (Id == Species::Luciolle ? 24 : 16);
    return Base + 8 * (Clamp(Level, 1, MaxLevel) - 1);
}

// Save migration and team editing use the same duplicate/index validation as the portable tests.
inline std::vector<int> NormalizeParty(const std::vector<int>& Input, int RosterCount, int Preferred = 0)
{
    std::vector<int> Result;
    for (int Index : Input)
    {
        bool Duplicate = false;
        for (int Existing : Result) if (Existing == Index) Duplicate = true;
        if (Index >= 0 && Index < RosterCount && !Duplicate && Result.size() < PartyCapacity) Result.push_back(Index);
    }
    if (Result.empty() && RosterCount > 0)
    {
        Result.push_back(Clamp(Preferred, 0, RosterCount - 1));
        for (int Index = 0; Index < RosterCount && Result.size() < PartyCapacity; ++Index)
            if (Index != Result[0]) Result.push_back(Index);
    }
    return Result;
}
}
