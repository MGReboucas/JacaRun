#include <iostream>
#include "include/GameManager.h"
#include "include/Player.h"

int main() {
    GameManager game;
    Player jaca;

    game.StartGame();

    // Simulando 10 "frames" (quadros) do jogo a rodar
    for (int frame = 1; frame <= 10; frame++) {
        std::cout << "\n--- Frame " << frame << " ---\n";
        
        // No frame 3, simulamos o jogador a tocar no ecrã para saltar
        if (frame == 3) {
            jaca.Jump();
        }

        jaca.Update();
        game.Update();

        std::cout << "Altura atual do Jaca (Y): " << jaca.GetPositionY() << "\n";
    }

    std::cout << "\n--- Fim da Simulacao ---\n";
    return 0;
}