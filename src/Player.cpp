#include "../include/Player.h"
#include <iostream>

Player::Player() {
    positionY = 0.0f; // Chão do mangue
    velocityY = 0.0f;
    gravity = -2.0f;  
    jumpForce = 8.0f; 
    isGrounded = true;
}

void Player::Update() {
    if (!isGrounded) {
        velocityY += gravity;         // A gravidade puxa para baixo
        positionY += velocityY;       // Atualiza a posição
    }

    // Verifica se bateu no chão
    if (positionY <= 0.0f) {
        positionY = 0.0f;
        velocityY = 0.0f;
        if (!isGrounded) {
            std::cout << "O Jaca tocou no chao do mangue.\n";
        }
        isGrounded = true;
    }
}

void Player::Jump() {
    if (isGrounded) {
        velocityY = jumpForce;
        isGrounded = false;
        std::cout << "O Jaca SALTOU! BOING!\n";
    }
}

float Player::GetPositionY() {
    return positionY;
}