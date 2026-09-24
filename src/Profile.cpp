#include "../include/Profile.h"
#include "../include/Balance.h"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <system_error>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

const std::array<Accessory, 4>& AccessoryCatalog() {
    static const std::array<Accessory, 4> catalog{{
        {"Jaca original", 0}, {"Bone do mangue", 15},
        {"Oculos tropicais", 35}, {"Chapeu de pescador", 60}
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
    if (accessory < 0 || accessory >= static_cast<int>(owned.size()) || !owned[accessory]) return false;
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
    if (magic != "JACARUN" || version != 1 || (input >> extra) ||
        candidate.coins < 0 || candidate.coins > limit || candidate.experience < 0 ||
        candidate.experience > limit || candidate.bestScore < 0 || candidate.bestScore > limit ||
        !std::isfinite(candidate.bestDistance) || candidate.bestDistance < 0 || candidate.bestDistance > limit ||
        mask > 15 || !(mask & 1) || candidate.equipped < 0 || candidate.equipped >= 4 ||
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
    output << "JACARUN 1\n" << coins << ' ' << experience << ' ' << bestScore << ' '
           << std::setprecision(17) << bestDistance << ' ' << mask << ' ' << equipped << '\n';
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
