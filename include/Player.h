#ifndef PLAYER_H
#define PLAYER_H

class Player {
private:
    float positionY;
    float velocityY;
    float gravity;
    float jumpForce;
    bool isGrounded;
    
    // Novas variáveis para o Slide
    bool isSliding;
    int slideDuration; // Quantos frames o Jaca fica agachado

public:
    Player();
    void Update();
    void Jump();
    void Slide(); // Nova ação
    float GetPositionY();
    bool IsSliding(); // Para sabermos a hitbox dele na colisão
};

#endif