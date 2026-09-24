#include "include/Balance.h"
#include "include/GameManager.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

namespace {
struct Options {
    bool demo = false;
    bool save = true;
    bool help = false;
    std::uint32_t seed = static_cast<std::uint32_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    std::filesystem::path savePath = "output/jacarun.save";
};

void Help() {
    std::cout << "JACARUN - prototipo de mecanicas no terminal\n"
              << "Uso: jacarun [--seed NUMERO] [--save ARQUIVO] [--no-save] [--demo] [--help]\n"
              << "Menu: jogar | loja | comprar ID | equipar ID | sair\n"
              << "Corrida: ENTER/correr | pular (p) | agachar (a) | pausa | menu | sair\n"
              << "Cada acao valida da corrida avanca 0.2 s. Pausa congela tudo.\n"
              << "--demo executa uma corrida automatica sem ler ou gravar progresso.\n";
}

bool ParseOptions(int argc, char** argv, Options& options) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--demo") options.demo = true;
        else if (arg == "--no-save") options.save = false;
        else if (arg == "--help") options.help = true;
        else if ((arg == "--seed" || arg == "--save") && i + 1 < argc) {
            const std::string value = argv[++i];
            if (arg == "--save") {
                if (value.empty()) return false;
                options.savePath = value;
            } else {
                if (value.empty() || value.find_first_not_of("0123456789") != std::string::npos) return false;
                try {
                    const auto seed = std::stoull(value);
                    if (seed > std::numeric_limits<std::uint32_t>::max()) return false;
                    options.seed = static_cast<std::uint32_t>(seed);
                } catch (...) { return false; }
            }
        } else return false;
    }
    if (options.demo) options.save = false;
    return true;
}

void Messages(GameManager& game) {
    for (const auto& message : game.TakeMessages()) std::cout << "  " << message << '\n';
}

void ShowProfile(const GameManager& game) {
    const auto& profile = game.GetProfile();
    std::cout << "Saldo: " << profile.coins << " moedas | Nivel: " << profile.GetLevel()
              << " | XP: " << profile.experience << " | Recorde: " << profile.bestScore
              << " pontos / " << std::fixed << std::setprecision(1) << profile.bestDistance << " m\n"
              << "Equipado: " << AccessoryCatalog()[profile.equipped].name << '\n';
}

void ShowShop(const GameManager& game) {
    ShowProfile(game);
    const auto& catalog = AccessoryCatalog();
    for (std::size_t i = 0; i < catalog.size(); ++i) {
        std::cout << i << " - " << catalog[i].name << " | " << catalog[i].price << " moedas"
                  << (game.GetProfile().owned[i] ? " | adquirido" : "") << '\n';
    }
    std::cout << "Use comprar ID ou equipar ID. Acessorios sao cosmeticos e aparecem no painel.\n";
}

void ShowRun(const GameManager& game) {
    std::cout << std::fixed << std::setprecision(1)
              << "\nDistancia " << game.GetDistance() << " m | Velocidade " << game.GetSpeed()
              << " m/s | Pontos " << game.GetScore() << " | Moedas " << game.GetRunCoins()
              << " | Combo " << game.GetCombo() << " x" << game.GetMultiplier() << '\n';
    std::cout << "Jaca: " << (game.GetPlayer().IsSliding() ? "agachado" :
                             game.GetPlayer().IsGrounded() ? "correndo" : "no ar")
              << " | Altura " << game.GetPlayer().GetPositionY()
              << " | Escudo " << game.GetShieldSeconds() << " s | Ima " << game.GetMagnetSeconds() << " s\n";
    int shown = 0;
    for (const auto& entity : game.GetLevel().GetEntities()) {
        const double remaining = entity.distance - game.GetDistance();
        std::cout << "  " << EntityName(entity.type) << ": " << remaining << " m ("
                  << remaining / game.GetSpeed() << " s)";
        if (!IsObstacle(entity.type)) std::cout << (entity.height > 1.0 ? " [no ar]" : " [baixo]");
        std::cout << '\n';
        if (++shown == 6) break;
    }
}

bool SaveProfile(GameManager& game, const Options& options) {
    if (!options.save) return true;
    std::string error;
    if (!game.GetProfile().Save(options.savePath, error)) {
        std::cerr << "ERRO: " << error << " O progresso continua apenas nesta sessao.\n";
        return false;
    }
    std::cout << "Progresso salvo.\n";
    return true;
}

bool ParseCommand(const std::string& line, std::string& command, int& id) {
    std::istringstream input(line);
    command.clear();
    input >> command;
    for (char& ch : command) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    if (command == "comprar" || command == "equipar") {
        if (!(input >> id)) return false;
    }
    std::string extra;
    return !(input >> extra);
}

int Demo(GameManager& game, const Options& options) {
    game.StartGame(options.seed);
    std::cout << "DEMO | Seed " << options.seed << " | sem salvamento\n";
    for (int step = 0; step < 1600 && game.IsPlaying(); ++step) {
        if (game.GetPlayer().IsGrounded() && !game.GetPlayer().IsSliding()) {
            for (const auto& entity : game.GetLevel().GetEntities()) {
                if (entity.distance - game.GetDistance() > 6) break;
                if (IsObstacle(entity.type) && !RequiresJump(entity.type)) { game.Slide(); break; }
                if (RequiresJump(entity.type) || (IsFood(entity.type) && entity.height > 1.0)) {
                    game.Jump();
                    break;
                }
            }
        }
        game.Update(0.05);
        Messages(game);
    }
    const bool survived = game.IsPlaying();
    game.EndRun("Fim da demonstracao.");
    Messages(game);
    ShowRun(game);
    ShowProfile(game);
    return survived ? 0 : 1;
}
}

int main(int argc, char** argv) {
    Options options;
    if (!ParseOptions(argc, argv, options)) { Help(); return 2; }
    if (options.help) { Help(); return 0; }
    GameManager game;
    if (options.demo) return Demo(game, options);

    std::string error;
    if (options.save && !game.GetProfile().Load(options.savePath, error)) {
        std::cerr << error << "\nUse --no-save para jogar sem alterar o arquivo, ou --save com outro caminho.\n";
        return 1;
    }
    Help();
    ShowProfile(game);
    bool saved = true;
    std::string line;
    while (true) {
        const auto state = game.GetState();
        if (state == GameState::Playing) ShowRun(game);
        else if (state == GameState::Paused) std::cout << "PAUSADO: pausa para continuar, menu ou sair.\n";
        std::cout << (state == GameState::Menu || state == GameState::GameOver ? "\nMenu> " : "\nCorrida> ");
        if (!std::getline(std::cin, line)) line = "sair";
        std::string command;
        int id = -1;
        if (!ParseCommand(line, command, id)) {
            std::cout << "Comando invalido. Use ajuda para consultar os controles.\n";
            continue;
        }
        if (command == "sair") {
            game.EndRun("Ate a proxima corrida!");
            Messages(game);
            saved = SaveProfile(game, options);
            break;
        }
        if (command == "ajuda") { Help(); continue; }
        if (command == "menu") {
            game.ReturnToMenu();
            Messages(game);
            saved = SaveProfile(game, options);
            ShowProfile(game);
            continue;
        }
        if (state == GameState::Menu || state == GameState::GameOver) {
            if (command == "jogar" || command == "j" || command == "reiniciar") {
                game.StartGame(options.seed++);
                Messages(game);
            } else if (command == "loja") ShowShop(game);
            else if (command == "comprar" || command == "equipar") {
                const bool ok = command == "comprar" ? game.BuyAccessory(id) : game.EquipAccessory(id);
                if (ok) {
                    std::cout << (command == "comprar" ? "Acessorio comprado!\n" : "Acessorio equipado!\n");
                    saved = SaveProfile(game, options);
                } else std::cout << "Operacao recusada: confira ID, saldo e itens adquiridos.\n";
                ShowShop(game);
            } else std::cout << "Use jogar, loja, comprar ID, equipar ID ou sair.\n";
            continue;
        }
        if (command == "pausa") { game.TogglePause(); continue; }
        if (state == GameState::Paused) { std::cout << "Continue a partida com pausa.\n"; continue; }
        bool accepted = true;
        if (command == "pular" || command == "p") {
            if (!game.Jump()) std::cout << "Pulo indisponivel: espere voltar ao chao e terminar o deslize.\n";
        } else if (command == "agachar" || command == "a") {
            if (!game.Slide()) std::cout << "Deslize indisponivel: espere voltar ao chao e terminar a acao.\n";
        } else if (!command.empty() && command != "correr") accepted = false;
        if (!accepted) { std::cout << "Comando invalido. Use ajuda.\n"; continue; }
        game.Update(Balance::TurnSeconds);
        Messages(game);
        if (game.GetState() == GameState::GameOver) {
            std::cout << "GAME OVER | " << game.GetScore() << " pontos | " << game.GetDistance() << " m\n";
            saved = SaveProfile(game, options);
            ShowProfile(game);
            std::cout << "Use jogar para tentar novamente ou loja para gastar suas moedas.\n";
        }
    }
    return saved ? 0 : 1;
}
