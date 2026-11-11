#pragma once
#include <algorithm>
#include <cmath>

struct EnvironmentBlend {
    int previous = 0;
    int current = 0;
    float progress = 1;
};

// Distance is the single clock: pause freezes the transition and restart resets it.
inline EnvironmentBlend EnvironmentAtDistance(double distance) {
    constexpr double segmentMeters = 500;
    constexpr double transitionMeters = 40;
    if (!std::isfinite(distance) || distance < segmentMeters) return {};
    const double lap = std::fmod(distance, segmentMeters * 3);
    const int current = static_cast<int>(lap / segmentMeters);
    const float t = static_cast<float>(std::clamp(std::fmod(lap, segmentMeters) / transitionMeters, 0.0, 1.0));
    return {(current + 2) % 3, current, t * t * (3 - 2 * t)};
}
