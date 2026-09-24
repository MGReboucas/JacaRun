#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

#include "Level.h"
#include "Player.h"
#include "Profile.h"
#include <cstdint>
#include <string>
#include <vector>

enum class GameState { Menu, Playing, Paused, GameOver };
enum class EntityOutcome { Pending, Passed, Collected, Missed, Hit, Shielded };
struct EntityResolution { Entity entity; EntityOutcome outcome; };

class GameManager {
public:
    void StartGame(std::uint32_t seed, bool procedural = true);
    void Update(double deltaTime);
    bool Jump();
    bool Slide();
    void TogglePause();
    void EndRun(const std::string& reason = "Corrida encerrada.");
    void ReturnToMenu();
    bool BuyAccessory(int id);
    bool EquipAccessory(int id);
    std::vector<std::string> TakeMessages();
    // Results from the most recent Update only. Bounded even without a visual consumer.
    const std::vector<EntityResolution>& GetFrameResolutions() const { return frameResolutions; }

    GameState GetState() const { return state; }
    bool IsPlaying() const { return state == GameState::Playing; }
    const Player& GetPlayer() const { return player; }
    const Level& GetLevel() const { return level; }
    const Profile& GetProfile() const { return profile; }
    Profile& GetProfile() { return profile; }
    double GetDistance() const { return distance; }
    double GetSpeed() const { return speed; }
    std::int64_t GetScore() const;
    int GetCombo() const { return combo; }
    int GetMultiplier() const;
    int GetRunCoins() const { return runCoins; }
    int GetFoods() const { return foods; }
    int GetObstaclesPassed() const { return obstaclesPassed; }
    double GetShieldSeconds() const { return shieldRemaining; }
    double GetMagnetSeconds() const { return magnetRemaining; }
    double GetFrenzySeconds() const { return frenzyRemaining; }
    int GetFrenzyMultiplier() const { return frenzyRemaining > 0 ? frenzyMultiplier : 1; }
    // Permite cenarios deterministas e futuros editores de percurso.
    void Spawn(EntityType type, double atDistance, double height = 0.0);

private:
    void Resolve(const Entity& entity);
    void ResetCombo();
    GameState state = GameState::Menu;
    Player player;
    Level level;
    Profile profile;
    double distance = 0.0;
    double speed = 0.0;
    std::int64_t foodScore = 0;
    double distanceScore = 0;
    double frenzyRemaining = 0;
    int frenzyMultiplier = 1;
    int combo = 0;
    double comboRemaining = 0.0;
    int runCoins = 0;
    int foods = 0;
    int obstaclesPassed = 0;
    double shieldRemaining = 0.0;
    double magnetRemaining = 0.0;
    std::vector<std::string> messages;
    std::vector<EntityResolution> frameResolutions;
};

#endif
