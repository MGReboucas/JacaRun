#include "../include/Balance.h"
#include "../include/GameManager.h"
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#define CHECK(condition) do { if (!(condition)) throw std::runtime_error( \
    std::string(__func__) + ":" + std::to_string(__LINE__) + " - " #condition); } while (false)

bool Near(double a, double b, double epsilon = 1e-7) { return std::abs(a - b) <= epsilon; }

void PlayerActions() {
    Player player;
    CHECK(player.IsGrounded());
    CHECK(player.Jump());
    CHECK(!player.Jump());
    CHECK(!player.Slide());
    player.Update(0.4);
    CHECK(player.GetPositionY() > Balance::GroundClearance);
    player.Update(0.6);
    CHECK(player.IsGrounded());
    CHECK(Near(player.GetPositionY(), 0));
    CHECK(player.Slide());
    CHECK(!player.Jump());
    CHECK(!player.Slide());
    player.Update(Balance::SlideSeconds / 2);
    CHECK(player.IsSliding());
    player.Update(Balance::SlideSeconds);
    CHECK(!player.IsSliding());
    CHECK(player.Jump());
    player.Reset();
    CHECK(player.IsGrounded() && !player.IsSliding());
}

void TimeBasedPhysics() {
    Player a, b;
    a.Jump();
    b.Jump();
    a.Update(0.4);
    for (int i = 0; i < 40; ++i) b.Update(0.01);
    CHECK(Near(a.GetPositionY(), b.GetPositionY()));
    a.Update(-1);
    a.Update(std::numeric_limits<double>::quiet_NaN());
    CHECK(Near(a.GetPositionY(), b.GetPositionY()));
}

void CrossingAndConsumption() {
    Level level;
    level.Reset(1, false);
    level.Spawn(EntityType::Ground, 5);
    level.Spawn(EntityType::High, 10);
    level.Spawn(EntityType::Coin, 20, 0.5);
    auto events = level.Crossed(0, 10);
    CHECK(events.size() == 2);
    CHECK(events[0].type == EntityType::Ground);
    CHECK(events[1].type == EntityType::High);
    CHECK(level.Crossed(10, 10).empty());
    CHECK(level.Crossed(10, 25).size() == 1);
    CHECK(level.GetEntities().empty());
    level.Spawn(EntityType::Coin, 25, 0.5);
    CHECK(level.Crossed(25, 26).size() == 1);
}

void GroundCollisions() {
    GameManager hit;
    hit.StartGame(1, false);
    hit.Spawn(EntityType::Ground, 5);
    hit.Update(1.0); // Cruza o limite em vez de depender de uma faixa aberta.
    CHECK(hit.GetState() == GameState::GameOver);
    CHECK(hit.GetDistance() < 5.1);
    GameManager safe;
    safe.StartGame(1, false);
    safe.Spawn(EntityType::Ground, 3);
    CHECK(safe.Jump());
    safe.Update(0.4);
    CHECK(safe.IsPlaying());
    CHECK(safe.GetObstaclesPassed() == 1);
    safe.Update(1);
    CHECK(safe.GetObstaclesPassed() == 1);
}

void HighCollisions() {
    GameManager safe;
    safe.StartGame(1, false);
    safe.Spawn(EntityType::High, 3);
    CHECK(safe.Slide());
    safe.Update(0.4);
    CHECK(safe.IsPlaying() && safe.GetObstaclesPassed() == 1);
    GameManager jumping;
    jumping.StartGame(1, false);
    jumping.Spawn(EntityType::High, 3);
    jumping.Jump();
    jumping.Update(0.4);
    CHECK(jumping.GetState() == GameState::GameOver);
    GameManager standing;
    standing.StartGame(1, false);
    standing.Spawn(EntityType::High, 3);
    standing.Update(0.4);
    CHECK(standing.GetState() == GameState::GameOver);
}

void FoodAndCombos() {
    GameManager game;
    game.StartGame(1, false);
    for (int i = 1; i <= 3; ++i) game.Spawn(EntityType::Crab, i, 0.5);
    game.Update(0.4);
    CHECK(game.GetFoods() == 3);
    CHECK(game.GetCombo() == 3 && game.GetMultiplier() == 2);
    CHECK(game.GetScore() == static_cast<std::int64_t>(game.GetDistance()) + 80);
    game.Spawn(EntityType::Fish, game.GetDistance() + 0.1, 2.0);
    game.Update(0.02);
    CHECK(game.GetCombo() == 0); // Perder alimento quebra a sequencia.
    CHECK(game.GetFoods() == 3);
    for (int i = 1; i <= 20; ++i) game.Spawn(EntityType::Crab, game.GetDistance() + i * 0.1, 0.5);
    game.Update(0.3);
    CHECK(game.GetMultiplier() == Balance::MaximumMultiplier);
    game.Update(Balance::ComboSeconds + 0.1);
    CHECK(game.GetCombo() == 0 && game.GetMultiplier() == 1);
}

void AirPickupsAndMagnet() {
    GameManager game;
    game.StartGame(1, false);
    game.Spawn(EntityType::Coin, 0.1, 2.0);
    game.Update(0.02);
    CHECK(game.GetRunCoins() == 0);
    game.Spawn(EntityType::Magnet, 0.3, 0.5);
    game.Spawn(EntityType::Coin, 0.4, 2.0);
    game.Update(0.03);
    CHECK(game.GetRunCoins() == 1);
    CHECK(game.GetMagnetSeconds() > 0);
    game.Update(Balance::MagnetSeconds + 0.1);
    game.Spawn(EntityType::Coin, game.GetDistance() + 0.1, 2.0);
    game.Update(0.02);
    CHECK(game.GetRunCoins() == 1);
    game.Spawn(EntityType::RareFish, game.GetDistance() + 3.0, 2.0);
    game.Jump();
    game.Update(0.4); // Cruza o peixe enquanto o personagem ainda esta no ar.
    CHECK(game.GetFoods() == 1);
}

void ShieldAbsorbsOnceAndExpires() {
    GameManager game;
    game.StartGame(1, false);
    game.Spawn(EntityType::Shield, 0.1, 0.5);
    game.Spawn(EntityType::Ground, 1);
    game.Update(0.15);
    CHECK(game.IsPlaying() && Near(game.GetShieldSeconds(), 0));
    game.Spawn(EntityType::Ground, 2);
    game.Update(0.1);
    CHECK(game.GetState() == GameState::GameOver);
    game.StartGame(1, false);
    game.Spawn(EntityType::Shield, 0.1, 0.5);
    game.Update(Balance::ShieldSeconds + 0.2);
    CHECK(Near(game.GetShieldSeconds(), 0));
    game.Spawn(EntityType::High, game.GetDistance() + 0.1);
    game.Update(0.02);
    CHECK(game.GetState() == GameState::GameOver);
}

void PauseAndRestart() {
    GameManager game;
    game.StartGame(1, false);
    game.Spawn(EntityType::Crab, 0.1, 0.5);
    game.Spawn(EntityType::Shield, 0.2, 0.5);
    game.Spawn(EntityType::Magnet, 0.3, 0.5);
    game.Update(0.1);
    game.Slide();
    const auto distance = game.GetDistance();
    const auto shield = game.GetShieldSeconds();
    const auto magnet = game.GetMagnetSeconds();
    game.TogglePause();
    CHECK(!game.Jump() && !game.Slide());
    game.Update(100);
    CHECK(Near(game.GetDistance(), distance));
    CHECK(Near(game.GetShieldSeconds(), shield));
    CHECK(Near(game.GetMagnetSeconds(), magnet));
    CHECK(game.GetCombo() == 1 && game.GetPlayer().IsSliding());
    game.TogglePause();
    game.Update(0.2);
    CHECK(game.GetDistance() > distance);
    game.EndRun();
    const auto xp = game.GetProfile().experience;
    game.StartGame(2, false);
    CHECK(game.GetCombo() == 0 && game.GetRunCoins() == 0 && game.GetScore() == 0);
    CHECK(Near(game.GetSpeed(), Balance::InitialSpeed));
    CHECK(Near(game.GetShieldSeconds(), 0) && Near(game.GetMagnetSeconds(), 0));
    CHECK(game.GetPlayer().IsGrounded() && !game.GetPlayer().IsSliding());
    CHECK(game.GetProfile().experience == xp);
    CHECK(game.GetLevel().GetEntities().empty());
}

void RewardsAndShop() {
    GameManager game;
    game.StartGame(1, false);
    game.Spawn(EntityType::Coin, 0.1, 0.5);
    game.Spawn(EntityType::Crab, 0.2, 0.5);
    game.Update(0.1);
    CHECK(game.GetProfile().coins == 0);
    CHECK(!game.BuyAccessory(1));
    game.TogglePause();
    game.EndRun();
    CHECK(game.GetProfile().coins == 1);
    CHECK(game.GetProfile().experience == 3);
    const auto record = game.GetProfile().bestScore;
    game.EndRun();
    game.ReturnToMenu();
    CHECK(game.GetProfile().coins == 1 && game.GetProfile().experience == 3);
    CHECK(game.GetProfile().bestScore == record);
    CHECK(!game.BuyAccessory(1) && !game.EquipAccessory(1));
    game.GetProfile().coins = 50;
    CHECK(game.BuyAccessory(1));
    CHECK(game.GetProfile().coins == 35);
    CHECK(!game.BuyAccessory(1));
    CHECK(game.GetProfile().coins == 35);
    CHECK(game.EquipAccessory(1) && game.GetProfile().equipped == 1);
    CHECK(!game.BuyAccessory(-1) && !game.EquipAccessory(4));
    game.GetProfile().experience = Balance::ExperiencePerLevel;
    CHECK(game.GetProfile().GetLevel() == 2);
}

void SpeedAndFrameIndependence() {
    GameManager a, b;
    a.StartGame(1, false);
    b.StartGame(1, false);
    a.Jump();
    b.Jump();
    a.Update(0.4);
    for (int i = 0; i < 4; ++i) b.Update(0.1);
    CHECK(Near(a.GetDistance(), b.GetDistance()));
    CHECK(Near(a.GetPlayer().GetPositionY(), b.GetPlayer().GetPositionY()));
    a.Update(200);
    CHECK(Near(a.GetSpeed(), Balance::MaximumSpeed));
    const auto distance = a.GetDistance();
    a.Update(std::numeric_limits<double>::infinity());
    a.Update(-1);
    CHECK(Near(a.GetDistance(), distance));
}

void SeedAndGeneration() {
    Level a, b, other;
    a.Reset(42);
    b.Reset(42);
    other.Reset(43);
    const auto initial = a.GetEntities();
    CHECK(initial.size() == b.GetEntities().size());
    for (std::size_t i = 0; i < initial.size(); ++i) {
        CHECK(initial[i].type == b.GetEntities()[i].type);
        CHECK(Near(initial[i].distance, b.GetEntities()[i].distance));
        if (i > 0) CHECK(initial[i].distance >= initial[i - 1].distance);
    }
    bool different = initial.size() != other.GetEntities().size();
    for (std::size_t i = 0; i < std::min(initial.size(), other.GetEntities().size()); ++i)
        different |= initial[i].type != other.GetEntities()[i].type ||
                     !Near(initial[i].distance, other.GetEntities()[i].distance);
    CHECK(different);
    for (int distance = 10; distance < 10000; distance += 10) {
        a.Crossed(distance - 10, distance);
        a.GenerateAhead(distance);
        CHECK(a.GetEntities().size() < 90); // Includes the complete last generated combination.
        CHECK(!a.GetEntities().empty());
        for (const auto& entity : a.GetEntities()) CHECK(entity.distance > distance);
    }
}

struct TempFiles {
    std::filesystem::path folder = std::filesystem::temp_directory_path() /
        ("jacarun-tests-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    TempFiles() { std::filesystem::create_directories(folder); }
    ~TempFiles() {
        std::error_code ec;
        std::filesystem::remove(folder / "profile.save.tmp", ec);
        std::filesystem::remove(folder / "profile.save", ec);
        std::filesystem::remove(folder / "blocker", ec);
        std::filesystem::remove(folder, ec);
    }
};

void PersistenceAndInvalidSave() {
    TempFiles files;
    const auto path = files.folder / "profile.save";
    Profile profile;
    std::string error;
    CHECK(profile.Load(path, error));
    profile.coins = 80;
    profile.experience = 650;
    profile.bestScore = 1234;
    profile.bestDistance = 123.45;
    CHECK(profile.Buy(2) && profile.Equip(2));
    CHECK(profile.Save(path, error));
    profile.coins = 99;
    CHECK(profile.Save(path, error)); // Substituicao de save existente.
    CHECK(!std::filesystem::exists(files.folder / "profile.save.tmp"));
    Profile loaded;
    CHECK(loaded.Load(path, error));
    CHECK(loaded.coins == 99 && loaded.experience == 650 && loaded.bestScore == 1234);
    CHECK(Near(loaded.bestDistance, 123.45) && loaded.owned[2] && loaded.equipped == 2);
    for (const std::string invalid : {
        "JACARUN 99\n0 0 0 0 1 0\n", "JACARUN 1\n-1 0 0 0 1 0\n",
        "JACARUN 1\n0 0 0 0 1 3\n", "JACARUN 1\n0 0 0 0 31 0\n",
        "JACARUN 1\n0 0 0 0 1 0 trailing\n", "truncado"}) {
        { std::ofstream out(path); out << invalid; }
        CHECK(!loaded.Load(path, error) && !error.empty());
        CHECK(loaded.coins == 99 && loaded.equipped == 2);
        std::ifstream in(path);
        const std::string preserved((std::istreambuf_iterator<char>(in)), {});
        CHECK(preserved == invalid);
    }
    { std::ofstream out(files.folder / "blocker"); out << "arquivo, nao pasta"; }
    CHECK(!profile.Save(files.folder / "blocker" / "profile.save", error));
    CHECK(!error.empty());
}

void PlayableProceduralRuns() {
    for (std::uint32_t seed = 0; seed < 100; ++seed) {
        GameManager game;
        game.StartGame(seed);
        for (int step = 0; step < 4000 && game.IsPlaying(); ++step) {
            if (game.GetPlayer().IsGrounded() && !game.GetPlayer().IsSliding()) {
                for (const auto& entity : game.GetLevel().GetEntities()) {
                    if ((entity.distance - game.GetDistance()) / game.GetSpeed() > 0.3) break;
                    if (entity.type == EntityType::High) { game.Slide(); break; }
                    if (entity.type == EntityType::Ground || (IsFood(entity.type) && entity.height > 1)) {
                        game.Jump();
                        break;
                    }
                }
            }
            game.Update(0.05);
            game.TakeMessages();
        }
        CHECK(game.IsPlaying());
        CHECK(game.GetDistance() > 4000 && game.GetObstaclesPassed() >= 90);
        CHECK(game.GetFoods() > 20 && game.GetRunCoins() > 40);
        CHECK(game.GetScore() > static_cast<std::int64_t>(game.GetDistance()));
    }
}

void ProgressiveEncounterSafety() {
    for (std::uint32_t seed = 0; seed < 100; ++seed) {
        Level level;
        level.Reset(seed);
        double previous = -100;
        int early = 0, late = 0, repeated = 0, alternating = 0;
        EntityType previousType = EntityType::Coin;
        for (int distance = 10; distance <= 6000; distance += 10) {
            for (const auto& entity : level.Crossed(distance - 10, distance)) {
                if (!IsObstacle(entity.type)) continue;
                // At maximum speed, enough time to finish either action and react again.
                const double seconds = (entity.distance - previous) / Balance::MaximumSpeed;
                CHECK(seconds >= 1.35 - 1e-7);
                CHECK(seconds > Balance::SlideSeconds + .4);
                CHECK(seconds > 2 * Balance::JumpForce / -Balance::Gravity + .4);
                if (entity.distance <= 1000) ++early;
                if (entity.distance >= 5000) ++late;
                if (previousType == entity.type) ++repeated;
                else ++alternating;
                previous = entity.distance;
                previousType = entity.type;
            }
            level.GenerateAhead(distance);
        }
        CHECK(late > early);
        CHECK(repeated > 5 && alternating > 5);
    }
}

int main() {
    const std::vector<std::pair<const char*, std::function<void()>>> tests = {
        {"acoes do jogador", PlayerActions}, {"fisica por tempo", TimeBasedPhysics},
        {"cruzamento e consumo unico", CrossingAndConsumption}, {"colisao terrestre", GroundCollisions},
        {"colisao superior", HighCollisions}, {"alimentos e combos", FoodAndCombos},
        {"coleta aerea e ima", AirPickupsAndMagnet}, {"escudo", ShieldAbsorbsOnceAndExpires},
        {"pausa e reinicio", PauseAndRestart}, {"recompensas e loja", RewardsAndShop},
        {"velocidade e passos de tempo", SpeedAndFrameIndependence}, {"geracao e seeds", SeedAndGeneration},
        {"persistencia e save invalido", PersistenceAndInvalidSave},
        {"100 percursos jogaveis", PlayableProceduralRuns},
        {"progressao e recuperacao entre obstaculos", ProgressiveEncounterSafety}
    };
    int failures = 0;
    for (const auto& test : tests) {
        try { test.second(); std::cout << "[OK] " << test.first << '\n'; }
        catch (const std::exception& error) { ++failures; std::cerr << "[FALHOU] " << error.what() << '\n'; }
    }
    std::cout << tests.size() - failures << "/" << tests.size() << " grupos passaram.\n";
    return failures == 0 ? 0 : 1;
}
