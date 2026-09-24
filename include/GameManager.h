#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

// Estados possíveis do jogo
enum GameState {
    MENU,
    PLAYING,
    GAMEOVER
};

class GameManager {
private:
    GameState currentState;
    float currentSpeed; // Velocidade em que o cenário rola
    int score;          // Pontuação do jogador

public:
    GameManager();
    void Update();      // Atualiza a lógica do jogo
    void StartGame();
    void GameOver();
    bool IsPlaying();
};

#endif