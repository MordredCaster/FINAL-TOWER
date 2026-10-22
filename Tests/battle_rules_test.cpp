#include "FTBattleRules.h"
#include <cassert>
#include <cmath>
#include <limits>
#include <iostream>

using namespace FinalTowerRules;

static Battle Fresh()
{
    return {{40, 40, 8, 2}, {30, 30, 6, 1}, Outcome::Ongoing, true};
}

int main()
{
    auto B = Fresh();
    assert(std::abs(B.RecruitmentChance() - 0.15f) < 0.0001f);
    assert(B.Attack() && B.Enemy.HP == 23 && B.Ally.HP == 36);
    assert(B.RecruitmentChance() > 0.15f);
    assert(B.TryRecruit(0.0f) && B.Result == Outcome::Recruited);
    const auto AllyHP = B.Ally.HP;
    assert(!B.TryRecruit(0.0f) && !B.Attack() && !B.Escape());
    assert(B.Ally.HP == AllyHP); // No duplicate reward or retaliation after recruitment.

    B = Fresh();
    B.Enemy.HP = 1;
    assert(B.Attack() && B.Result == Outcome::Victory && B.Ally.HP == 40);
    assert(!B.TryRecruit(0.0f) && B.RecruitmentChance() == 0.0f);

    B = Fresh();
    B.Ally.HP = 1;
    assert(B.TryRecruit(0.999f) && B.Result == Outcome::Defeat && B.Ally.HP == 0);
    assert(!B.Attack());

    B = Fresh();
    B.Recruitable = false;
    assert(!B.TryRecruit(0.0f) && B.Ally.HP == 40);
    B.Recruitable = true;
    assert(!B.TryRecruit(-1.0f) && !B.TryRecruit(1.0f));
    assert(!B.TryRecruit(std::numeric_limits<float>::quiet_NaN()));
    assert(B.Escape() && B.Result == Outcome::Escaped && B.Ally.HP == 40);

    // Recruitment improves as HP drops and never becomes certain.
    float Previous = 0.0f;
    for (int HP = 30; HP > 0; --HP)
    {
        B = Fresh();
        B.Enemy.HP = HP;
        const float Chance = B.RecruitmentChance();
        assert(Chance >= Previous && Chance < 0.8f);
        auto Success = B;
        assert(Success.TryRecruit(Chance - 0.00001f));
        assert(Success.Result == Outcome::Recruited);
        auto Failure = B;
        assert(Failure.TryRecruit(Chance));
        assert(Failure.Result == Outcome::Ongoing && Failure.Ally.HP == 36);
        Previous = Chance;
    }
    B = Fresh();
    B.Ally.Attack = 0;
    B.Enemy.Defense = 99;
    assert(B.Attack() && B.Enemy.HP == 29); // No combat deadlock from armor.
    std::cout << "Battle rules: all checks passed\n";
}
