#ifndef LEVEL_H
#define LEVEL_H

#include <cstdint>
#include <random>
#include <vector>

enum class EntityType { Ground, High, Coin, Crab, Fish, RareFish, Shield, Magnet };

struct Entity {
    EntityType type;
    double distance; // Coordenada no mundo: o jogador avanca ate ela.
    double height;
};

const char* EntityName(EntityType type);
bool IsObstacle(EntityType type);
bool IsFood(EntityType type);

class Level {
public:
    void Reset(std::uint32_t seed, bool procedural = true);
    void GenerateAhead(double playerDistance);
    void Spawn(EntityType type, double distance, double height = 0.0);
    // Retira eventos cruzados em ordem, incluindo a fronteira de colisao.
    std::vector<Entity> Crossed(double previousDistance, double currentDistance);
    const std::vector<Entity>& GetEntities() const { return entities; }

private:
    std::mt19937 random{1};
    std::vector<Entity> entities;
    double nextEncounter = 30.0;
    int encounterCount = 0;
    bool generate = true;
};

#endif
