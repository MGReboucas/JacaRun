#pragma once
#include <algorithm>
#include <cmath>

inline constexpr double EnvironmentSegmentMeters = 1200;
inline constexpr double EnvironmentTransitionMeters = 180;

// New scenery enters from the right; foreground crosses later and faster.
inline float EnvironmentCoverage(float progress, float x, bool foreground) {
    const float phase = foreground ? std::clamp((progress-.35f)/.45f,0.0f,1.0f) : progress;
    const float feather = foreground ? 48.0f : 120.0f;
    const float edge = 480+feather-phase*(480+2*feather);
    const float t = std::clamp((x-edge+feather)/(2*feather),0.0f,1.0f);
    return t*t*(3-2*t);
}

struct EnvironmentBlend {
    int previous = 0;
    int current = 0;
    float progress = 1;
};

// Distance is the single clock: pause freezes the transition and restart resets it.
inline EnvironmentBlend EnvironmentAtDistance(double distance) {
    constexpr double segmentMeters = EnvironmentSegmentMeters;
    constexpr double transitionMeters = EnvironmentTransitionMeters;
    if (!std::isfinite(distance) || distance < segmentMeters) return {};
    const double lap = std::fmod(distance, segmentMeters * 3);
    const int current = static_cast<int>(lap / segmentMeters);
    const float t = static_cast<float>(std::clamp(std::fmod(lap, segmentMeters) / transitionMeters, 0.0, 1.0));
    return {(current + 2) % 3, current, t};
}
