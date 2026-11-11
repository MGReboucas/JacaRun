#pragma once
#include <algorithm>
#include <cmath>

// Presentation only: never changes the collision shape or simulation clock.
struct CharacterMotion {
    float time = 0, phase = 0, slide = 0, landing = 0, lift = 0;
    bool grounded = true;
    float bite = 0;
    float lean = 0, hurt = 0, defeat = 0;
    void Hit() { hurt = .36f; }
    void Eat() { bite = .42f; }
    float Mouth() const { return std::sin(3.14159265f * std::clamp(bite / .42f, 0.0f, 1.0f)); }
    void Reset() { *this = {}; }
    void Update(float dt, bool active, bool onGround, bool sliding, double speed,
                double velocityY = 0, bool defeated = false) {
        if (!active || !std::isfinite(dt) || dt <= 0) return;
        dt = std::min(dt, .1f);
        time += dt;
        bite = std::max(0.0f, bite - dt);
        hurt = std::max(0.0f, hurt - dt);
        defeat = defeated ? std::min(2.0f, defeat + dt) : 0;
        // Capped cadence avoids unreadable frame flicker at late-game speeds.
        if(onGround && !sliding && !defeated)
            phase = std::fmod(phase + dt * std::clamp(7.5f + static_cast<float>(speed) * .24f, 8.0f, 17.0f), 6.2831853f);
        const float targetLean=onGround?0:std::clamp(static_cast<float>(-velocityY)*.55f,-8.0f,10.0f);
        lean += (targetLean-lean)*(1-std::exp(-12*dt));
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
