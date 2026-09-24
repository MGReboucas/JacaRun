#ifndef PLAYER_H
#define PLAYER_H

class Player {
private:
    float positionY;
    float velocityY;
    float gravity;
    float jumpForce;
    bool isGrounded;

public:
    Player();
    void Update(); // Vamos simplificar sem deltaTime por agora
    void Jump();
    float GetPositionY();
};

#endif