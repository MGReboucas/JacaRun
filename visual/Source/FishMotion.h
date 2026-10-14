#pragma once
#include <algorithm>

// A single leap, synchronized to the encounter rather than a repeating idle bob.
// At contact the fish is exactly at the height used by the gameplay collision.
struct FishPose {
    float height;
    float angle;
    float splash;
    bool airborne;
};
inline FishPose FishLeap(double remaining, float waterHeight, float catchHeight) {
    constexpr double reach = 24.0;
    const float phase = static_cast<float>(std::clamp(remaining / reach, -1.0, 1.0));
    const float arc = 1 - phase * phase;
    return {waterHeight + (catchHeight - waterHeight) * arc,
            -.95f * phase, std::max(0.0f, 1 - arc * 3),
            remaining < reach && remaining > -reach};
}
