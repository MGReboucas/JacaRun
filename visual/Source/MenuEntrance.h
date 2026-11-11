#pragma once
#include <algorithm>
#include <cmath>

struct MenuEntrance {
    float time = 0;
    static constexpr float Duration = 3.6f;
    void Update(float dt) { time = std::min(Duration, time + std::clamp(dt, 0.0f, .1f)); }
    void Finish() { time = Duration; }
    float CameraProgress() const {
        const float t = std::clamp((time - .15f) / 1.8f, 0.0f, 1.0f);
        return std::clamp(t*t*t*(t*(t*6-15)+10),0.0f,1.0f);
    }
    float Zoom() const { return 1 + 2.0f*(1-CameraProgress()); }
    float Progress() const {
        const float t = std::clamp((time - .35f) / 1.3f, 0.0f, 1.0f);
        return t*t*(3-2*t);
    }
    float Hop() const {
        const float t=std::clamp((time-.85f)/.8f,0.0f,1.0f);
        return time>=1.65f?0:std::sin(t*3.14159265f)*35;
    }
};
