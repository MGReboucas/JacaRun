#ifndef PROFILE_H
#define PROFILE_H

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>

struct Accessory {
    const char* name;
    std::int64_t price;
};

const std::array<Accessory, 4>& AccessoryCatalog();

struct Profile {
    std::int64_t coins = 0;
    std::int64_t experience = 0;
    std::int64_t bestScore = 0;
    double bestDistance = 0.0;
    std::array<bool, 4> owned{{true, false, false, false}};
    int equipped = 0;

    std::int64_t GetLevel() const;
    bool Buy(int accessory);
    bool Equip(int accessory);
    // Falha de leitura preserva o perfil em memoria.
    bool Load(const std::filesystem::path& path, std::string& error);
    bool Save(const std::filesystem::path& path, std::string& error) const;
};

#endif
