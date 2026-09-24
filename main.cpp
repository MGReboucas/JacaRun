#include <iostream>
#include "include/GameManager.h"
#include "include/Player.h"
#include "include/Level.h"

int main() {
    GameManager game;
    Player jaca;
    Level mangue;

    game.StartGame();

    // Aumentamos os frames para 18 para caber as duas ações
    for (int frame = 1; frame <= 18; frame++) {
        std::cout << "\n--- Frame " << frame << " ---\n";
        
        // 1. Gera um obstáculo ALTO no frame 2
        if (frame == 2) { mangue.SpawnObstacle(15.0f, HIGH); }

        // O jogador reage ao galho agachando no frame 4
        if (frame == 4) { jaca.Slide(); }

        // 2. Gera um obstáculo no CHÃO no frame 10
        if (frame == 10) { mangue.SpawnObstacle(15.0f, GROUND); }

        // O jogador reage à raiz saltando no frame 12
        if (frame == 12) { jaca.Jump(); }

        jaca.Update();
        game.Update();
        mangue.Update(10.0f);

        // Feedback visual da hitbox do Jaca
        std::cout << "Estado do Jaca -> Altura (Y): " << jaca.GetPositionY();
        if (jaca.IsSliding()) std::cout << " | [AGACHADO]";
        std::cout << "\n";

        // --- SISTEMA DE COLISÃO DUPLO ---
        if (mangue.HasObstacle() && mangue.GetObstaclePosition() > 0.0f && mangue.GetObstaclePosition() < 5.0f) {
            
            if (mangue.GetObstacleType() == GROUND) {
                // Se é raiz, tem que pular (Y >= 4)
                if (jaca.GetPositionY() < 4.0f) {
                    std::cout << "\n*** BAM! O JACA TROPECOU NA RAIZ! ***\n";
                    game.GameOver();
                    break;
                } else {
                    std::cout << "*** BOA! O Jaca passou por cima da raiz! ***\n";
                }
            } 
            else if (mangue.GetObstacleType() == HIGH) {
                // Se é galho, tem que estar deslizando E no chão
                if (!jaca.IsSliding() || jaca.GetPositionY() > 0.0f) {
                    std::cout << "\n*** BAM! O JACA BATEU A CABECA NO GALHO! ***\n";
                    game.GameOver();
                    break;
                } else {
                    std::cout << "*** BOA! O Jaca deslizou por baixo do galho! ***\n";
                }
            }
        }
    }

    std::cout << "\n--- Fim da Simulacao ---\n";
    return 0;
}