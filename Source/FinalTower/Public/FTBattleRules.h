#pragma once

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
};

struct Battle
{
    Fighter Ally;
    Fighter Enemy;
    Outcome Result = Outcome::Ongoing;
    bool Recruitable = true;

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
        const int Damage = Enemy.Attack > Ally.Defense ? Enemy.Attack - Ally.Defense : 1;
        Ally.HP = Ally.HP > Damage ? Ally.HP - Damage : 0;
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
};
}
