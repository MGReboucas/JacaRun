#include "../include/Level.h"
#include "../include/Balance.h"
#include <algorithm>
#include <cmath>

const char* EntityName(EntityType type) {
    switch (type) {
        case EntityType::Ground: return "raiz (pule)";
        case EntityType::High: return "galho (agache)";
        case EntityType::Coin: return "moeda";
        case EntityType::Crab: return "caranguejo";
        case EntityType::Fish: return "peixe";
        case EntityType::RareFish: return "peixe raro";
        case EntityType::Shield: return "escudo";
        case EntityType::Magnet: return "ima";
    }
    return "item";
}

bool IsObstacle(EntityType type) {
    return type == EntityType::Ground || type == EntityType::High;
}

bool IsFood(EntityType type) {
    return type == EntityType::Crab || type == EntityType::Fish || type == EntityType::RareFish;
}

void Level::Reset(std::uint32_t seed, bool procedural) {
    random.seed(seed);
    entities.clear();
    nextEncounter = 30.0;
    encounterCount = 0;
    nextId = 1;
    generate = procedural;
    GenerateAhead(0.0);
}

void Level::Spawn(EntityType type, double distance, double height) {
    if (!std::isfinite(distance) || distance < 0.0 || !std::isfinite(height) || height < 0.0) return;
    entities.push_back({type, distance, height, nextId++});
    std::stable_sort(entities.begin(), entities.end(), [](const Entity& a, const Entity& b) {
        return a.distance < b.distance;
    });
}

void Level::GenerateAhead(double playerDistance) {
    if (!generate) return;
    while (nextEncounter < playerDistance + Balance::LookAhead) {
        const bool ground = random() % 2 == 0;
        Spawn(ground ? EntityType::Ground : EntityType::High, nextEncounter);
        // Recompensa junto ao obstaculo valoriza a acao correta.
        Spawn(ground ? EntityType::Fish : EntityType::Crab, nextEncounter, ground ? 2.0 : 0.5);
        for (int coin = 1; coin <= 3; ++coin) {
            Spawn(EntityType::Coin, nextEncounter + 12.0 + coin * 2.0, 0.5);
        }
        const bool rare = random() % 5 == 0;
        Spawn(rare ? EntityType::RareFish : EntityType::Crab, nextEncounter + 25.0, rare ? 2.0 : 0.5);
        ++encounterCount;
        if (encounterCount % 4 == 0) {
            Spawn(encounterCount % 8 == 0 ? EntityType::Magnet : EntityType::Shield,
                  nextEncounter + 32.0, 0.5);
        }
        // Espacamento permite terminar uma acao antes do proximo obstaculo.
        const double minimumGap = std::max(48.0, 72.0 - nextEncounter * 0.015);
        nextEncounter += minimumGap + static_cast<double>(random() % 13);
    }
}

std::vector<Entity> Level::Crossed(double previousDistance, double currentDistance) {
    std::vector<Entity> result;
    if (currentDistance < previousDistance) return result;
    auto end = std::upper_bound(entities.begin(), entities.end(), currentDistance,
        [](double distance, const Entity& entity) { return distance < entity.distance; });
    for (auto it = entities.begin(); it != end; ++it) {
        if (it->distance >= previousDistance) result.push_back(*it);
    }
    entities.erase(entities.begin(), end);
    return result;
}
