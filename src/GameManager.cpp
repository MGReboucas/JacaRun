#include "../include/GameManager.h"
#include <iostream>

GameManager::GameManager() {
    currentState = MENU;
    currentSpeed = 10.0f;
    score = 0;
}

void GameManager::StartGame() {
    currentState = PLAYING;
    score = 0;
    std::cout << "Jogo Iniciado! O Jaca comecou a correr!\n";
}

void GameManager::GameOver() {
    currentState = GAMEOVER;
    std::cout << "Game Over! Bateu num obstaculo do mangue.\n";
}

void GameManager::Update() {
    if (currentState == PLAYING) {
        // Aumenta a pontuação e a velocidade aos poucos
        score += 1;
        currentSpeed += 0.01f;
        std::cout << "Correndo... Distancia: " << score << "m | Velocidade: " << currentSpeed << "\n";
    }
}

bool GameManager::IsPlaying() {
    return currentState == PLAYING;
}