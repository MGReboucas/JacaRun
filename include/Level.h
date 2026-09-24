#ifndef LEVEL_H
#define LEVEL_H

class Level {
private:
    float obstaclePositionX; // Distância do obstáculo em relação ao Jaca
    bool hasObstacle;        // Existe um obstáculo na tela?

public:
    Level();
    void Update(float currentSpeed); // Move o obstáculo
    void SpawnObstacle(float startDistance); // Cria um novo obstáculo
    float GetObstaclePosition();
    bool HasObstacle();
};

#endif