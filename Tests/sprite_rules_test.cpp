#include "FTSpriteRules.h"
#include <cassert>
#include <iostream>

using namespace FinalTowerSpriteRules;
int main()
{
    assert(FromScreenAxes(1, 0, Facing::South) == Facing::East);
    assert(FromScreenAxes(-1, 0, Facing::South) == Facing::West);
    assert(FromScreenAxes(0, 1, Facing::South) == Facing::North);
    assert(FromScreenAxes(0, -1, Facing::North) == Facing::South);
    assert(FromScreenAxes(0, 0, Facing::West) == Facing::West);
    assert(FromScreenAxes(0.01f, -0.01f, Facing::North) == Facing::North);
    assert(FromScreenAxes(0.8f, 0.2f, Facing::South) == Facing::East);
    assert(FromScreenAxes(-0.1f, 0.9f, Facing::South) == Facing::North);
    std::cout << "Camera-relative sprite directions: all checks passed\n";
}
