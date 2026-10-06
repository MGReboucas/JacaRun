#include "../include/Profile.h"
#include "../include/Balance.h"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <system_error>
#include <algorithm>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

const std::array<ProgressGoal, MissionCount>& MissionCatalog() {
    static const std::array<ProgressGoal, MissionCount> goals{{
        {"Primeiros passos", "Percorra 250 m", GoalMetric::Distance, 250, 80},
        {"Bolso cheio", "Colete 15 moedas", GoalMetric::Coins, 15, 100},
        {"Jogo de cintura", "Supere 8 obstaculos", GoalMetric::Obstacles, 8, 100},
        {"Passeio no mangue", "Percorra 500 m", GoalMetric::Distance, 500, 120},
        {"Hora do lanche", "Coma 5 alimentos", GoalMetric::Foods, 5, 100},
        {"Caminho livre", "Supere 15 obstaculos", GoalMetric::Obstacles, 15, 150},
        {"Explorador", "Percorra 750 m", GoalMetric::Distance, 750, 160},
        {"Tesouro do mangue", "Colete 30 moedas", GoalMetric::Coins, 30, 150},
        {"Bom apetite", "Coma 8 alimentos", GoalMetric::Foods, 8, 140}
    }};
    return goals;
}

const std::array<ProgressGoal, AchievementCount>& AchievementCatalog() {
    static const std::array<ProgressGoal, AchievementCount> goals{{
        {"O mangue e seu", "Percorra 500 m no total", GoalMetric::Distance, 500, 200},
        {"Colecionador", "Colete 100 moedas no total", GoalMetric::Coins, 100, 250},
        {"Sem perder o passo", "Supere 25 obstaculos no total", GoalMetric::Obstacles, 25, 250},
        {"Banquete tropical", "Coma 30 alimentos no total", GoalMetric::Foods, 30, 300},
        {"Sempre de volta", "Termine 10 corridas de 50 m ou mais", GoalMetric::Runs, 10, 400},
        {"Rei do percurso", "Alcance 2.000 m em uma corrida", GoalMetric::BestDistance, 2000, 600}
    }};
    return goals;
}

std::int64_t RunProgress::Value(GoalMetric metric) const {
    switch (metric) {
    case GoalMetric::Distance: case GoalMetric::BestDistance: return distance;
    case GoalMetric::Coins: return coins;
    case GoalMetric::Obstacles: return obstacles;
    case GoalMetric::Foods: return foods;
    case GoalMetric::Runs: return distance >= 50 ? 1 : 0;
    }
    return 0;
}

std::int64_t Profile::AchievementProgress(int id) const {
    if (id < 0 || id >= AchievementCount) return 0;
    const auto& goal = AchievementCatalog()[id];
    const auto value = goal.metric == GoalMetric::BestDistance ? static_cast<std::int64_t>(bestDistance)
                      : lifetime[static_cast<int>(goal.metric)];
    return std::min(goal.target, value);
}

ProgressRewards Profile::ApplyRunProgress(const RunProgress& run) {
    ProgressRewards rewards;
    constexpr std::int64_t limit = 1000000000000LL;
    for (int i = 0; i < 5; ++i)
        lifetime[i] = std::min(limit, lifetime[i] + std::clamp<std::int64_t>(run.Value(static_cast<GoalMetric>(i)), 0, limit));
    bestDistance = std::max(bestDistance, static_cast<double>(run.distance));
    for (int slot = 0; slot < MissionSlots; ++slot) {
        const auto& goal = MissionCatalog()[missionIds[slot]];
        missionProgress[slot] += std::clamp<std::int64_t>(run.Value(goal.metric), 0, goal.target);
        if (missionProgress[slot] >= goal.target) {
            rewards.coins += goal.reward;
            rewards.missions |= 1u << slot;
            missionsCompleted = std::min(limit, missionsCompleted + 1);
            missionIds[slot] = (missionIds[slot] + MissionSlots) % MissionCount;
            missionProgress[slot] = 0; // The next card starts on the next run; no reward cascade.
        }
    }
    for (int id = 0; id < AchievementCount; ++id) {
        if (!(achievements & (1u << id)) && AchievementProgress(id) >= AchievementCatalog()[id].target) {
            rewards.achievements |= 1u << id;
            rewards.coins += AchievementCatalog()[id].reward;
            achievements |= 1u << id;
        }
    }
    coins += rewards.coins;
    return rewards;
}

const std::array<Accessory, ShopItemCount>& AccessoryCatalog() {
    static const std::array<Accessory, ShopItemCount> catalog{{
        {"Jaca original", 0, "O classico do mangue. Visual."},
        {"Bone do mangue", 2000, "Estilo de quem corre todo dia. Visual."},
        {"Oculos tropicais", 5000, "Lentes escuras, sangue frio. Visual."},
        {"Chapeu de pescador", 9000, "Para veteranos do mangue. Visual."},
        {"Bandana vermelha", 15000, "Uma marca de persistencia. Visual."},
        {"Coroa do mangue", 30000, "Conquiste seu reinado. Visual."},
        {"Capacete lunar", 50000, "Para quem foi longe demais. Visual."},
        {"Folego do combo", 12000, "Combo dura +2 s sem comer."},
        {"Ima duradouro", 20000, "Cada ima coletado dura +4 s."},
        {"Frenesi prolongado", 35000, "Bonus dos peixes dura +3 s."}
    }};
    return catalog;
}

std::int64_t Profile::GetLevel() const {
    return 1 + experience / Balance::ExperiencePerLevel;
}

bool Profile::Buy(int accessory) {
    if (accessory < 0 || accessory >= static_cast<int>(owned.size()) || owned[accessory]) return false;
    const auto price = AccessoryCatalog()[accessory].price;
    if (coins < price) return false;
    coins -= price;
    owned[accessory] = true;
    return true;
}

bool Profile::Equip(int accessory) {
    if (accessory < 0 || accessory >= CosmeticCount || !owned[accessory]) return false;
    equipped = accessory;
    return true;
}

bool Profile::Load(const std::filesystem::path& path, std::string& error) {
    error.clear();
    std::error_code ec;
    const bool exists = std::filesystem::exists(path, ec);
    if (ec) { error = "Nao foi possivel consultar o save: " + ec.message(); return false; }
    if (!exists) return true;
    std::ifstream input(path);
    Profile candidate;
    std::string magic;
    int version = 0;
    unsigned int mask = 0;
    if (!(input >> magic >> version >> candidate.coins >> candidate.experience >> candidate.bestScore
                >> candidate.bestDistance >> mask >> candidate.equipped)) {
        error = "Save incompleto ou ilegivel. O arquivo foi preservado.";
        return false;
    }
    std::string extra;
    constexpr std::int64_t limit = 1000000000000LL;
    const unsigned int allowedMask = version == 1 ? 15u : (1u << ShopItemCount) - 1;
    const int allowedEquipped = version == 1 ? 4 : CosmeticCount;
    bool progressValid = true;
    if (version == 3) {
        if (!(input >> candidate.achievements >> candidate.missionsCompleted)) progressValid = false;
        for (auto& value : candidate.lifetime)
            if (!(input >> value) || value < 0 || value > limit) progressValid = false;
        for (int slot = 0; slot < MissionSlots; ++slot) {
            auto& id = candidate.missionIds[slot];
            auto& progress = candidate.missionProgress[slot];
            if (!(input >> id >> progress) || id < 0 || id >= MissionCount || id % MissionSlots != slot)
                progressValid = false;
            else if (progress < 0 || progress >= MissionCatalog()[id].target) progressValid = false;
        }
        if (candidate.achievements >= (1u << AchievementCount) || candidate.missionsCompleted < 0 ||
            candidate.missionsCompleted > limit) progressValid = false;
    }
    if (magic != "JACARUN" || (version != 1 && version != 2 && version != 3) || !progressValid || (input >> extra) ||
        candidate.coins < 0 || candidate.coins > limit || candidate.experience < 0 ||
        candidate.experience > limit || candidate.bestScore < 0 || candidate.bestScore > limit ||
        !std::isfinite(candidate.bestDistance) || candidate.bestDistance < 0 || candidate.bestDistance > limit ||
        mask > allowedMask || !(mask & 1) || candidate.equipped < 0 || candidate.equipped >= allowedEquipped ||
        !(mask & (1u << candidate.equipped))) {
        error = "Save invalido ou de versao incompativel. O arquivo foi preservado.";
        return false;
    }
    for (std::size_t i = 0; i < candidate.owned.size(); ++i) candidate.owned[i] = (mask & (1u << i)) != 0;
    *this = candidate;
    return true;
}

bool Profile::Save(const std::filesystem::path& path, std::string& error) const {
    error.clear();
    std::error_code ec;
    if (!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path(), ec);
    if (ec) { error = "Nao foi possivel criar a pasta do save: " + ec.message(); return false; }
    auto temporary = path;
    temporary += ".tmp";
    std::ofstream output(temporary, std::ios::trunc);
    unsigned int mask = 0;
    for (std::size_t i = 0; i < owned.size(); ++i) if (owned[i]) mask |= 1u << i;
    output << "JACARUN 3\n" << coins << ' ' << experience << ' ' << bestScore << ' '
           << std::setprecision(17) << bestDistance << ' ' << mask << ' ' << equipped << '\n';
    output << achievements << ' ' << missionsCompleted;
    for (const auto value : lifetime) output << ' ' << value;
    output << '\n';
    for (int slot = 0; slot < MissionSlots; ++slot)
        output << missionIds[slot] << ' ' << missionProgress[slot] << '\n';
    output.flush();
    const bool written = output.good();
    output.close();
    if (!written || output.fail()) {
        error = "Falha ao escrever o save. O save anterior foi preservado.";
        return false;
    }
#ifdef _WIN32
    const bool replaced = MoveFileExW(temporary.c_str(), path.c_str(),
                                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    const bool replaced = std::rename(temporary.c_str(), path.c_str()) == 0;
#endif
    if (!replaced) {
        error = "Falha ao substituir o save. O save anterior foi preservado.";
        return false;
    }
    return true;
}
