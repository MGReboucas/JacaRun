#ifndef LEVEL_H
#define LEVEL_H

// Tipos de obstáculos no mangue
enum ObstacleType {
    GROUND, // Raiz (Precisa pular)
    HIGH    // Galho ou Barco (Precisa agachar)
};

class Level {
private:
    float obstaclePositionX;
    bool hasObstacle;
    ObstacleType currentType;

public:
    Level();
    void Update(float currentSpeed);
    void SpawnObstacle(float startDistance, ObstacleType type); // Agora pede o tipo
    float GetObstaclePosition();
    bool HasObstacle();
    ObstacleType GetObstacleType();
};

#endif