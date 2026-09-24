#include "../include/Player.h"
#include "../include/Balance.h"
#include <algorithm>
#include <cmath>

void Player::Reset() {
    positionY = velocityY = slideRemaining = 0.0;
    grounded = true;
}

void Player::Update(double deltaTime) {
    if (!std::isfinite(deltaTime) || deltaTime <= 0.0) return;
    slideRemaining = std::max(0.0, slideRemaining - deltaTime);
    if (!grounded) {
        positionY += velocityY * deltaTime + 0.5 * Balance::Gravity * deltaTime * deltaTime;
        velocityY += Balance::Gravity * deltaTime;
        if (positionY <= 0.0) {
            positionY = velocityY = 0.0;
            grounded = true;
        }
    }
}

bool Player::Jump() {
    if (!grounded || IsSliding()) return false;
    velocityY = Balance::JumpForce;
    grounded = false;
    return true;
}

bool Player::Slide() {
    if (!grounded || IsSliding()) return false;
    slideRemaining = Balance::SlideSeconds;
    return true;
}
