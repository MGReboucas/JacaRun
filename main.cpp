#include <iostream>
#include "include/GameManager.h"
#include "include/Player.h"
#include "include/Level.h" // Adicionamos o Level

int main() {
    GameManager game;
    Player jaca;
    Level mangue;

    game.StartGame();

    // Aumentamos para 12 frames para ver o obstáculo passar
    for (int frame = 1; frame <= 12; frame++) {
        std::cout << "\n--- Frame " << frame << " ---\n";
        
        // Criamos o obstáculo no frame 2
        if (frame == 2) {
            mangue.SpawnObstacle(20.0f);
        }

        // Simulação do pulo do jogador no momento certo (frame 4)
        // Experimente mudar para um frame diferente depois para ver o Jaca bater!
        if (frame == 4) {
            jaca.Jump();
        }

        // Atualizamos todo mundo
        jaca.Update();
        game.Update();
        mangue.Update(10.0f); // Passamos uma velocidade fixa para simplificar a simulação

        std::cout << "Altura do Jaca (Y): " << jaca.GetPositionY() << "\n";

        // --- SISTEMA DE COLISÃO SIMPLES ---
        if (mangue.HasObstacle()) {
            // Se o obstáculo está muito perto (entre 0 e 5 metros)
            if (mangue.GetObstaclePosition() > 0.0f && mangue.GetObstaclePosition() < 5.0f) {
                // E o Jaca está no chão (Altura Y muito baixa)
                if (jaca.GetPositionY() < 4.0f) {
                    std::cout << "\n*** BAM! O JACA BATEU NA RAIZ! ***\n";
                    game.GameOver();
                    break; // Encerra o loop do jogo
                } else {
                    std::cout << "*** BOA! O Jaca passou por cima da raiz! ***\n";
                }
            }
        }
    }

    std::cout << "\n--- Fim da Simulacao ---\n";
    return 0;
}