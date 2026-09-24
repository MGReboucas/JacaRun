#include "../include/Level.h"
#include <iostream>

Level::Level() {
    obstaclePositionX = 0.0f;
    hasObstacle = false;
    currentType = GROUND;
}

void Level::SpawnObstacle(float startDistance, ObstacleType type) {
    obstaclePositionX = startDistance;
    hasObstacle = true;
    currentType = type;
    
    if (type == GROUND) {
        std::cout << "[Cenario] Uma RAIZ NO CHAO apareceu a " << startDistance << " metros!\n";
    } else {
        std::cout << "[Cenario] Um GALHO ALTO apareceu a " << startDistance << " metros!\n";
    }
}

void Level::Update(float currentSpeed) {
    if (hasObstacle) {
        obstaclePositionX -= (currentSpeed * 0.5f); 

        if (obstaclePositionX < -2.0f) {
            hasObstacle = false;
            std::cout << "[Cenario] Obstaculo deixado para tras!\n";
        } else {
            std::cout << "-> Obstaculo aproximando... Distancia: " << obstaclePositionX << "m\n";
        }
    }
}

float Level::GetObstaclePosition() { return obstaclePositionX; }
bool Level::HasObstacle() { return hasObstacle; }
ObstacleType Level::GetObstacleType() { return currentType; }