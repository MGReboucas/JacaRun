#include "../include/Level.h"
#include <iostream>

Level::Level() {
    obstaclePositionX = 0.0f;
    hasObstacle = false;
}

void Level::SpawnObstacle(float startDistance) {
    obstaclePositionX = startDistance;
    hasObstacle = true;
    std::cout << "[Cenario] Uma raiz de mangue apareceu a " << startDistance << " metros!\n";
}

void Level::Update(float currentSpeed) {
    if (hasObstacle) {
        // O obstáculo se aproxima do Jaca subtraindo a velocidade da distância
        // Multiplicamos por 0.5 apenas para simular melhor no terminal frame a frame
        obstaclePositionX -= (currentSpeed * 0.5f); 

        if (obstaclePositionX < -2.0f) {
            // O obstáculo passou direto pelo Jaca
            hasObstacle = false;
            std::cout << "[Cenario] O Jaca deixou o obstaculo para tras!\n";
        } else {
            std::cout << "-> Raiz aproximando... Distancia: " << obstaclePositionX << "m\n";
        }
    }
}

float Level::GetObstaclePosition() {
    return obstaclePositionX;
}

bool Level::HasObstacle() {
    return hasObstacle;
}