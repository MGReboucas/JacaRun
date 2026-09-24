#ifndef PLAYER_H
#define PLAYER_H

class Player {
public:
    void Reset();
    void Update(double deltaTime);
    bool Jump();
    bool Slide();
    double GetPositionY() const { return positionY; }
    bool IsSliding() const { return slideRemaining > 0.0; }
    bool IsGrounded() const { return grounded; }

private:
    double positionY = 0.0;
    double velocityY = 0.0;
    double slideRemaining = 0.0;
    bool grounded = true;
};

#endif
