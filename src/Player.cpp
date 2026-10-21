#include "../include/Player.h"
#include "../include/Balance.h"
#include <algorithm>
#include <cmath>

void Player::Reset() {
    positionY = velocityY = slideRemaining = 0.0;
    grounded = true;
    diving = false;
}

void Player::Update(double deltaTime, double speed) {
    if (!std::isfinite(deltaTime) || deltaTime <= 0.0) return;
    if (!std::isfinite(speed) || speed <= 0.0) return;
    // Action progress follows world travel; slow running must not shorten clearance.
    deltaTime *= std::clamp(speed, Balance::InitialSpeed, Balance::MaximumSpeed) / Balance::ActionSpeed;
    slideRemaining = std::max(0.0, slideRemaining - deltaTime);
    if (!grounded) {
        if (diving) {
            const double landing = (-velocityY - std::sqrt(velocityY * velocityY - 2 * Balance::Gravity * positionY)) / Balance::Gravity;
            if (landing <= deltaTime) {
                positionY = velocityY = 0;
                grounded = true;
                diving = false;
                slideRemaining = std::max(0.0, Balance::SlideSeconds - (deltaTime - landing));
                return;
            }
        }
        positionY += velocityY * deltaTime + 0.5 * Balance::Gravity * deltaTime * deltaTime;
        velocityY += Balance::Gravity * deltaTime;
        if (positionY <= 0.0) {
            positionY = velocityY = 0.0;
            grounded = true;
        }
    }
}

bool Player::Jump() {
    if (!grounded) return false;
    slideRemaining = 0;
    velocityY = Balance::JumpForce;
    grounded = false;
    return true;
}

bool Player::Slide() {
    if (diving || IsSliding()) return false;
    if (!grounded) {
        diving = true;
        velocityY = std::min(velocityY, -Balance::DiveSpeed);
        return true;
    }
    slideRemaining = Balance::SlideSeconds;
    return true;
}
