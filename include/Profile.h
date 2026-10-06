#ifndef PROFILE_H
#define PROFILE_H

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>

struct Accessory {
    const char* name;
    std::int64_t price;
    const char* description;
};

inline constexpr int CosmeticCount = 7;
inline constexpr int ShopItemCount = 10;
inline constexpr int ComboUpgrade = 7, MagnetUpgrade = 8, FrenzyUpgrade = 9;
const std::array<Accessory, ShopItemCount>& AccessoryCatalog();

enum class GoalMetric { Distance, Coins, Obstacles, Foods, Runs, BestDistance };
struct ProgressGoal {
    const char* title;
    const char* description;
    GoalMetric metric;
    std::int64_t target;
    int reward;
};
inline constexpr int MissionSlots = 3, MissionCount = 9, AchievementCount = 6;
const std::array<ProgressGoal, MissionCount>& MissionCatalog();
const std::array<ProgressGoal, AchievementCount>& AchievementCatalog();
struct RunProgress {
    std::int64_t distance = 0, coins = 0, obstacles = 0, foods = 0;
    std::int64_t Value(GoalMetric metric) const;
};
struct ProgressRewards {
    int coins = 0;
    unsigned int missions = 0, achievements = 0;
};

struct Profile {
    std::int64_t coins = 0;
    std::int64_t experience = 0;
    std::int64_t bestScore = 0;
    double bestDistance = 0.0;
    std::array<bool, ShopItemCount> owned{{true}};
    int equipped = 0;
    std::array<int, MissionSlots> missionIds{{0, 1, 2}};
    std::array<std::int64_t, MissionSlots> missionProgress{};
    std::array<std::int64_t, 5> lifetime{};
    unsigned int achievements = 0;
    std::int64_t missionsCompleted = 0;

    std::int64_t AchievementProgress(int id) const;
    ProgressRewards ApplyRunProgress(const RunProgress& run);

    std::int64_t GetLevel() const;
    bool Buy(int accessory);
    bool Equip(int accessory);
    // Falha de leitura preserva o perfil em memoria.
    bool Load(const std::filesystem::path& path, std::string& error);
    bool Save(const std::filesystem::path& path, std::string& error) const;
};

#endif
