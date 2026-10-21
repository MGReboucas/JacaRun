#pragma once
#include <algorithm>

struct MenuEntrance {
    float time = 0;
    static constexpr float Duration = 3.6f;
    void Update(float dt) { time = std::min(Duration, time + std::clamp(dt, 0.0f, .1f)); }
    void Finish() { time = Duration; }
    float CameraProgress() const {
        const float t = std::clamp((time - .6f) / 3.0f, 0.0f, 1.0f);
        return t*t*(3-2*t);
    }
    float Zoom() const { return 1 + 2.0f*(1-CameraProgress()); }
    float Progress() const {
        const float t = std::clamp((time - .35f) / 2.0f, 0.0f, 1.0f);
        return t*t*(3-2*t);
    }
};
