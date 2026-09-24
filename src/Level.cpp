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
    previousPattern = -1;
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
        // Difficulty follows this run, never the saved profile: retries start gently.
        const double intensity = std::clamp(nextEncounter / 1800.0, 0.0, 1.0);
        const int available = nextEncounter < 180 ? 2 : nextEncounter < 550 ? 4 : 6;
        int pattern = static_cast<int>(random() % available);
        if (pattern == previousPattern) pattern = (pattern + 1) % available;
        previousPattern = pattern;
        // Singles, alternating pairs, repeated jumps and three-action combinations.
        static constexpr EntityType patterns[6][3] = {
            {EntityType::Ground, EntityType::Ground, EntityType::Ground},
            {EntityType::High, EntityType::High, EntityType::High},
            {EntityType::Ground, EntityType::High, EntityType::Ground},
            {EntityType::High, EntityType::Ground, EntityType::High},
            {EntityType::Ground, EntityType::Ground, EntityType::High},
            {EntityType::High, EntityType::Ground, EntityType::High}
        };
        const int count = pattern < 2 ? 1 : pattern < 4 ? 2 : 3;
        // Using the maximum speed guarantees recovery time even while accelerating.
        const double actionGap = Balance::MaximumSpeed * (1.65 - .30 * intensity);
        double at = nextEncounter;
        for (int action = 0; action < count; ++action) {
            const auto type = patterns[pattern][action];
            const bool jump = type == EntityType::Ground;
            Spawn(type, at);
            Spawn(jump ? EntityType::Fish : EntityType::Crab, at, jump ? 2.0 : .5);
            // Coin trails preview the required action; no optional jump before a branch.
            for (int coin = -1; coin <= 1; ++coin)
                Spawn(EntityType::Coin, at + coin * 2.0, jump ? 1.8 : .5);
            if (action + 1 < count) at += actionGap;
        }
        // A short reward/recovery lane separates combinations.
        for (int coin = 0; coin < 3; ++coin)
            Spawn(EntityType::Coin, at + 16 + coin * 3, .5);
        Spawn(random() % 5 == 0 ? EntityType::RareFish : EntityType::Crab, at + 22, .5);
        ++encounterCount;
        if (encounterCount % 4 == 0) {
            Spawn(encounterCount % 8 == 0 ? EntityType::Magnet : EntityType::Shield,
                  at + 25.0, 0.5);
        }
        // Espacamento permite terminar uma acao antes do proximo obstaculo.
        nextEncounter = at + 58.0 - 18.0 * intensity + static_cast<double>(random() % 9);
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
