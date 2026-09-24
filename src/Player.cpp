#include "../include/Player.h"
#include <iostream>

Player::Player() {
    positionY = 0.0f;
    velocityY = 0.0f;
    gravity = -2.0f;  
    jumpForce = 8.0f; 
    isGrounded = true;
    
    isSliding = false;
    slideDuration = 0;
}

void Player::Update() {
    // Física da Gravidade
    if (!isGrounded) {
        velocityY += gravity;
        positionY += velocityY;
    }

    if (positionY <= 0.0f) {
        positionY = 0.0f;
        velocityY = 0.0f;
        isGrounded = true;
    }

    // Lógica do Slide (Agachar)
    if (isSliding) {
        slideDuration--; // Diminui o tempo restante do deslize
        if (slideDuration <= 0) {
            isSliding = false;
            std::cout << "[Jaca] O Jaca levantou e voltou a correr normalmente.\n";
        }
    }
}

void Player::Jump() {
    // Só salta se estiver no chão e NÃO estiver agachado
    if (isGrounded && !isSliding) {
        velocityY = jumpForce;
        isGrounded = false;
        std::cout << "[Acao] O Jaca SALTOU! BOING!\n";
    }
}

void Player::Slide() {
    if (isGrounded && !isSliding) {
        isSliding = true;
        slideDuration = 3; // O deslize dura 3 frames nesta simulação
        std::cout << "[Acao] O Jaca AGACHOU! VRAUU!\n";
    }
}

float Player::GetPositionY() {
    return positionY;
}

bool Player::IsSliding() {
    return isSliding;
}