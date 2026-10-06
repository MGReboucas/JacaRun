#pragma once
#include <algorithm>
#include <cmath>

// Presentation only: never changes the collision shape or simulation clock.
struct CharacterMotion {
    float time = 0, phase = 0, slide = 0, landing = 0, lift = 0;
    bool grounded = true;
    void Reset() { *this = {}; }
    void Update(float dt, bool active, bool onGround, bool sliding, double speed) {
        if (!active || !std::isfinite(dt) || dt <= 0) return;
        dt = std::min(dt, .1f);
        time += dt;
        phase += dt * (9.0f + static_cast<float>(speed) * .36f);
        if (!grounded && onGround) landing = 1;
        if (grounded && !onGround) lift = 1;
        grounded = onGround;
        slide += ((sliding ? 1.0f : 0.0f) - slide) * (1 - std::exp(-26 * dt));
        landing = std::max(0.0f, landing - dt * 4.5f);
        lift = std::max(0.0f, lift - dt * 5);
    }
    float Width() const { return 1 + .09f * slide + .14f * landing - .07f * lift; }
    float Height() const { return 1 - .54f * slide - .20f * landing + .12f * lift; }
};
