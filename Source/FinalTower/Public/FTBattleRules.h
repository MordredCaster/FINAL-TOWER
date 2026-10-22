#pragma once
#include "FTProgressionRules.h"

// Engine-independent rules, also exercised by Tests/battle_rules_test.cpp.
namespace FinalTowerRules
{
enum class Outcome { Ongoing, Victory, Recruited, Defeat, Escaped };

struct Fighter
{
    int MaxHP = 1;
    int HP = 1;
    int Attack = 1;
    int Defense = 0;
    int MaxMP = 0;
    int MP = 0;
    int Magic = 0;
};

struct Battle
{
    Fighter Ally;
    Fighter Enemy;
    Outcome Result = Outcome::Ongoing;
    bool Recruitable = true;
    Species AllySpecies = Species::Mousse;
    int AllyLevel = 1;
    int GuardPower = 0;
    int GuardTurns = 0;
    int WeakenPower = 0;
    int WeakenTurns = 0;

    bool CanAct() const
    {
        return Result == Outcome::Ongoing && Ally.HP > 0 && Enemy.HP > 0;
    }

    float RecruitmentChance() const
    {
        if (!CanAct() || !Recruitable || Enemy.MaxHP <= 0) return 0.0f;
        const float Ratio = static_cast<float>(Enemy.HP) / Enemy.MaxHP;
        const float Missing = Ratio < 0.0f ? 1.0f : (Ratio > 1.0f ? 0.0f : 1.0f - Ratio);
        return 0.15f + Missing * 0.65f;
    }

    void Retaliate()
    {
        if (!CanAct()) return;
        const int AttackPower = Enemy.Attack - (WeakenTurns > 0 ? WeakenPower : 0);
        const int Armor = Ally.Defense + (GuardTurns > 0 ? GuardPower : 0);
        const int Damage = AttackPower > Armor ? AttackPower - Armor : 1;
        Ally.HP = Ally.HP > Damage ? Ally.HP - Damage : 0;
        if (GuardTurns > 0) --GuardTurns;
        if (WeakenTurns > 0) --WeakenTurns;
        if (Ally.HP == 0) Result = Outcome::Defeat;
    }

    bool Attack()
    {
        if (!CanAct()) return false;
        const int Damage = Ally.Attack > Enemy.Defense ? Ally.Attack - Enemy.Defense : 1;
        Enemy.HP = Enemy.HP > Damage ? Enemy.HP - Damage : 0;
        if (Enemy.HP == 0) Result = Outcome::Victory;
        else Retaliate();
        return true;
    }

    bool TryRecruit(float Roll)
    {
        // A non-recruitable creature or invalid random input must not consume a turn.
        if (!CanAct() || !Recruitable || !(Roll >= 0.0f && Roll < 1.0f)) return false;
        if (Roll < RecruitmentChance()) Result = Outcome::Recruited;
        else Retaliate();
        return true;
    }

    bool Escape()
    {
        if (!CanAct()) return false;
        Result = Outcome::Escaped;
        return true;
    }

    bool CanUseSkill(SkillId Technique) const
    {
        const Skill* Entry = FindSkill(Technique);
        return CanAct() && Entry && HasLearned(AllySpecies, AllyLevel, Technique) && Ally.MP >= Entry->MPCost &&
            !(Entry->Effect == SkillEffect::Heal && Ally.HP >= Ally.MaxHP);
    }

    bool UseSkill(SkillId Technique)
    {
        if (!CanUseSkill(Technique)) return false;
        const Skill& Entry = *FindSkill(Technique);
        Ally.MP -= Entry.MPCost;
        switch (Entry.Effect)
        {
        case SkillEffect::Heal:
            Ally.HP = Clamp(Ally.HP + Entry.Power + Ally.Magic, 0, Ally.MaxHP);
            break;
        case SkillEffect::Guard:
            GuardPower = Entry.Power;
            GuardTurns = Entry.Duration;
            break;
        case SkillEffect::Weaken:
            WeakenPower = Entry.Power;
            WeakenTurns = Entry.Duration;
            break;
        default:
        {
            const bool Magic = Entry.Effect == SkillEffect::Magical;
            const int Damage = Clamp((Magic ? Ally.Magic : Ally.Attack) + Entry.Power -
                (Magic ? Enemy.Defense / 2 : Enemy.Defense), 1, 1000000);
            Enemy.HP = Enemy.HP > Damage ? Enemy.HP - Damage : 0;
            break;
        }
        }
        if (Enemy.HP == 0) Result = Outcome::Victory;
        else Retaliate();
        return true;
    }

    bool SwitchAlly(Fighter Replacement, Species Id, int Level)
    {
        const bool KnockedOut = Ally.HP <= 0;
        if ((Result != Outcome::Ongoing && !(Result == Outcome::Defeat && KnockedOut)) ||
            Enemy.HP <= 0 || Replacement.HP <= 0 || Replacement.HP > Replacement.MaxHP) return false;
        Ally = Replacement;
        AllySpecies = Id;
        AllyLevel = Clamp(Level, 1, MaxLevel);
        GuardPower = GuardTurns = 0; // Personal protection never transfers to a different monster.
        Result = Outcome::Ongoing;
        if (!KnockedOut) Retaliate(); // A forced replacement after KO is free.
        return true;
    }
};
}
