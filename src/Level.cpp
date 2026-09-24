#include "../include/Level.h"
#include "../include/Balance.h"
#include <algorithm>
#include <cmath>

const char* EntityName(EntityType type) {
    switch (type) {
        case EntityType::Ground: return "raiz (pule)";
        case EntityType::High: return "galho (agache)";
        case EntityType::Log: return "tronco caido (pule)";
        case EntityType::Rock: return "pedra (pule)";
        case EntityType::Vine: return "cipo baixo (agache)";
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
    return RequiresJump(type) || type == EntityType::High || type == EntityType::Vine;
}

bool RequiresJump(EntityType type) {
    return type == EntityType::Ground || type == EntityType::Log || type == EntityType::Rock;
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

void Level::GenerateAhead(double playerDistance, double score) {
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
        const double pressure = Balance::ScorePressure(score);
        const int baseCount = pattern < 2 ? 1 : pattern < 4 ? 2 : 3;
        const int count = score >= 20000 ? std::max(baseCount, 3 + static_cast<int>(pressure * 5)) : baseCount;
        // Fixed action travel leaves recovery distance even at extreme speed.
        const double actionGap = Balance::ActionSpeed * (1.65 - .30 * intensity) - 2.4 * pressure;
        double at = nextEncounter;
        for (int action = 0; action < count; ++action) {
            auto type = patterns[pattern][action % 3];
            const bool jump = RequiresJump(type);
            if (at >= 300) {
                const auto variant = random() % (jump ? 3 : 2);
                type = jump ? (variant == 0 ? EntityType::Ground : variant == 1 ? EntityType::Log : EntityType::Rock)
                            : (variant == 0 ? EntityType::High : EntityType::Vine);
            }
            Spawn(type, at);
            if (jump) {
                // Sample the actual distance-based parabola, with takeoff 6 m before the root.
                for (int coin = 0; coin < 4; ++coin) {
                    const double offset = -4.0 + coin * 5.5;
                    const double t = (offset + 6.0) / Balance::ActionSpeed;
                    const double y = Balance::JumpForce * t + .5 * Balance::Gravity * t * t;
                    Spawn(EntityType::Coin, at + offset, .5 + y);
                }
                Spawn(random() % 5 == 0 ? EntityType::RareFish : EntityType::Fish, at + 20.5, .5);
            } else {
                for (int coin = 0; coin < 3; ++coin)
                    Spawn(EntityType::Coin, at - 4.0 + coin * 6.0, .3);
                Spawn(EntityType::Crab, at + 14.5, .5);
            }
            if (action + 1 < count) at += actionGap;
        }
        // A short reward/recovery lane separates combinations.
        ++encounterCount;
        if (encounterCount % 4 == 0) {
            Spawn(encounterCount % 8 == 0 ? EntityType::Magnet : EntityType::Shield,
                  at + 29.0, 0.5);
        }
        // Espacamento permite terminar uma acao antes do proximo obstaculo.
        const double recovery = 58.0 - 18.0 * intensity - 10.0 * pressure;
        nextEncounter = at + std::max(encounterCount % 4 == 0 ? 40.0 : 30.0, recovery)
                           + static_cast<double>(random() % 9) * (1.0 - pressure);
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
