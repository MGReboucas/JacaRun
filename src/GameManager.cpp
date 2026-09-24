#include "../include/GameManager.h"
#include "../include/Balance.h"
#include <algorithm>
#include <cmath>
#include <utility>

void GameManager::StartGame(std::uint32_t seed, bool procedural) {
    // Uma corrida ativa precisa ser encerrada para contabilizar suas recompensas.
    if (state == GameState::Playing || state == GameState::Paused) return;
    player.Reset();
    level.Reset(seed, procedural);
    distance = 0.0;
    speed = Balance::InitialSpeed;
    foodScore = 0;
    distanceScore = frenzyRemaining = 0;
    frenzyMultiplier = 1;
    combo = runCoins = foods = obstaclesPassed = 0;
    comboRemaining = shieldRemaining = magnetRemaining = 0.0;
    messages.clear();
    frameResolutions.clear();
    state = GameState::Playing;
    messages.emplace_back("Jaca no mangue! Pule as raizes e agache nos galhos.");
}

std::int64_t GameManager::GetScore() const {
    return static_cast<std::int64_t>(distanceScore) + foodScore;
}

int GameManager::GetMultiplier() const {
    return std::min(Balance::MaximumMultiplier, 1 + combo / Balance::FoodsPerMultiplier);
}

bool GameManager::Jump() {
    return IsPlaying() && player.Jump();
}

bool GameManager::Slide() {
    return IsPlaying() && player.Slide();
}

void GameManager::TogglePause() {
    if (state == GameState::Playing) state = GameState::Paused;
    else if (state == GameState::Paused) state = GameState::Playing;
}

void GameManager::ResetCombo() {
    if (combo > 0) messages.emplace_back("Combo encerrado.");
    combo = 0;
    comboRemaining = 0.0;
}

void GameManager::Update(double deltaTime) {
    frameResolutions.clear();
    if (!IsPlaying() || !std::isfinite(deltaTime) || deltaTime <= 0.0) return;
    // Passos curtos mantem fisica e colisoes consistentes mesmo com frames longos.
    while (deltaTime > 1e-9 && IsPlaying()) {
        const double dt = std::min(deltaTime, Balance::PhysicsStep);
        deltaTime -= dt;
        speed = Balance::RunSpeed(distance, static_cast<double>(GetScore()));
        player.Update(dt, speed);
        shieldRemaining = std::max(0.0, shieldRemaining - dt);
        magnetRemaining = std::max(0.0, magnetRemaining - dt);
        if (combo > 0) {
            comboRemaining -= dt;
            if (comboRemaining <= 0.0) ResetCombo();
        }
        const double previous = distance;
        distance += speed * dt;
        const double boosted = std::min(dt, frenzyRemaining);
        distanceScore += speed * (dt + boosted * (frenzyMultiplier - 1));
        frenzyRemaining = std::max(0.0, frenzyRemaining - dt);
        if (frenzyRemaining == 0) frenzyMultiplier = 1;
        for (const Entity& entity : level.Crossed(previous, distance)) {
            Resolve(entity);
            if (!IsPlaying()) break;
        }
        if (IsPlaying()) level.GenerateAhead(distance, static_cast<double>(GetScore()));
    }
}

void GameManager::Spawn(EntityType type, double atDistance, double height) {
    if (IsPlaying() && atDistance >= distance) level.Spawn(type, atDistance, height);
}

void GameManager::Resolve(const Entity& entity) {
    if (IsObstacle(entity.type)) {
        const bool safe = RequiresJump(entity.type)
            ? player.GetPositionY() >= Balance::GroundClearance
            : player.IsGrounded() && player.IsSliding();
        if (!safe) {
            if (shieldRemaining > 0.0) {
                frameResolutions.push_back({entity, EntityOutcome::Shielded});
                shieldRemaining = 0.0;
                frenzyRemaining = 0;
                frenzyMultiplier = 1;
                ResetCombo();
                messages.emplace_back("Escudo absorveu a batida!");
            } else {
                frameResolutions.push_back({entity, EntityOutcome::Hit});
                EndRun(std::string("O Jaca bateu: ") + EntityName(entity.type));
            }
        } else {
            frameResolutions.push_back({entity, EntityOutcome::Passed});
            ++obstaclesPassed;
            messages.emplace_back(RequiresJump(entity.type) ? "Boa! Passou por cima." : "Boa! Deslizou por baixo.");
        }
        return;
    }

    const double reach = entity.type == EntityType::RareFish ? .10 :
                         entity.type == EntityType::Fish ? .22 : Balance::PickupReach;
    const bool reachable = std::abs(player.GetPositionY() + 0.5 - entity.height) <= reach;
    if (entity.type == EntityType::Coin && (reachable || magnetRemaining > 0.0)) {
        frameResolutions.push_back({entity, EntityOutcome::Collected});
        ++runCoins;
        messages.emplace_back("+1 moeda");
    } else if (IsFood(entity.type)) {
        if (!reachable) {
            frameResolutions.push_back({entity, EntityOutcome::Missed});
            ResetCombo(); messages.emplace_back("Alimento perdido."); return;
        }
        frameResolutions.push_back({entity, EntityOutcome::Collected});
        ++foods;
        ++combo;
        comboRemaining = Balance::ComboSeconds + (profile.owned[ComboUpgrade] ? 2.0 : 0.0);
        const int base = entity.type == EntityType::Crab ? Balance::CrabPoints
                       : entity.type == EntityType::Fish ? Balance::FishPoints : Balance::RareFishPoints;
        const int points = base * GetMultiplier();
        foodScore += points;
        if (entity.type == EntityType::Fish || entity.type == EntityType::RareFish) {
            const bool rare = entity.type == EntityType::RareFish;
            frenzyMultiplier = std::max(GetFrenzyMultiplier(), rare ? 3 : 2);
            frenzyRemaining = std::max(frenzyRemaining, (rare ? 10.0 : 6.0) +
                                      (profile.owned[FrenzyUpgrade] ? 3.0 : 0.0));
            messages.emplace_back("FRENESI: pontos de distancia x" + std::to_string(frenzyMultiplier));
        }
        messages.emplace_back(std::string(EntityName(entity.type)) + ": +" + std::to_string(points) +
                              " pontos | combo " + std::to_string(combo) + " x" + std::to_string(GetMultiplier()));
    } else if (reachable && entity.type == EntityType::Shield) {
        frameResolutions.push_back({entity, EntityOutcome::Collected});
        shieldRemaining = Balance::ShieldSeconds;
        messages.emplace_back("Escudo: protege de uma batida por ate 12 segundos.");
    } else if (reachable && entity.type == EntityType::Magnet) {
        frameResolutions.push_back({entity, EntityOutcome::Collected});
        magnetRemaining = Balance::MagnetSeconds + (profile.owned[MagnetUpgrade] ? 4.0 : 0.0);
        messages.emplace_back("Ima ativado!");
    } else {
        frameResolutions.push_back({entity, EntityOutcome::Missed});
    }
}

void GameManager::EndRun(const std::string& reason) {
    if (state != GameState::Playing && state != GameState::Paused) return;
    state = GameState::GameOver; // Garante que a recompensa seja creditada uma unica vez.
    const auto xp = static_cast<std::int64_t>(distance / 10.0) + foods * 3LL + obstaclesPassed * 5LL;
    const auto oldLevel = profile.GetLevel();
    profile.coins += runCoins;
    profile.experience += xp;
    if (GetScore() > profile.bestScore) {
        profile.bestScore = GetScore();
        messages.emplace_back("NOVO RECORDE!");
    }
    profile.bestDistance = std::max(profile.bestDistance, distance);
    messages.push_back(reason);
    messages.emplace_back("Recompensas: " + std::to_string(runCoins) + " moedas e " + std::to_string(xp) + " XP.");
    if (profile.GetLevel() > oldLevel) messages.emplace_back("SUBIU DE NIVEL! Nivel " + std::to_string(profile.GetLevel()));
}

void GameManager::ReturnToMenu() {
    if (state == GameState::Playing || state == GameState::Paused) EndRun();
    state = GameState::Menu;
}

bool GameManager::BuyAccessory(int id) {
    return (state == GameState::Menu || state == GameState::GameOver) && profile.Buy(id);
}

bool GameManager::EquipAccessory(int id) {
    return (state == GameState::Menu || state == GameState::GameOver) && profile.Equip(id);
}

std::vector<std::string> GameManager::TakeMessages() {
    auto result = std::move(messages);
    messages.clear();
    return result;
}
