#pragma once

namespace FinalTowerSpriteRules
{
enum class Facing { South, West, East, North };

// Inputs are relative to the camera. Preserve facing when stopped; avoid diagonal flicker.
inline Facing FromScreenAxes(float Right, float Forward, Facing Previous)
{
    const float R = Right < 0 ? -Right : Right;
    const float F = Forward < 0 ? -Forward : Forward;
    if (R < 0.05f && F < 0.05f) return Previous;
    if (R > F) return Right > 0 ? Facing::East : Facing::West;
    return Forward > 0 ? Facing::North : Facing::South;
}

inline const char* DirectionName(Facing Value)
{
    switch (Value)
    {
    case Facing::West: return "west";
    case Facing::East: return "east";
    case Facing::North: return "north";
    default: return "south";
    }
}
}
