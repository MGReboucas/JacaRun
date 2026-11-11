# Animações do Jaca — 0.9.2

## Alterações

- 24 poses em três atlas: corrida (8), descanso/entrada (8) e ações (8).
- Corrida com cadência limitada, variação de compressão/alongamento, inclinação e poeira sincronizada às passadas. A densidade de pixels dos frames de corrida é preservada para reduzir mudanças de tamanho.
- Menu com respiração, piscadas, aceno e ajuste do boné. A entrada inclui preparação, salto da água, respingos, aterrissagem e saudação. A câmera se afasta antes do salto e a marca aparece gradualmente; os controles permanecem disponíveis.
- Pulo com subida, ápice e queda; inclinação filtrada pela velocidade vertical. Deslize e mergulho mantêm a postura baixa mesmo durante uma coleta.
- Mordida aberta e mastigação, incluindo coleta no ar. Reação de escudo, impacto e queda com estrelas de tontura. O placar aparece após 0,72 s, sem atrasar o crédito das recompensas.
- A animação de derrota leva o desenho até o chão mesmo quando a colisão ocorreu no ar. Isso não altera a simulação nem as colisões.
- Pausa congela os relógios de animação. Reinício limpa inclinação, reação, mastigação e derrota.

## Verificações

- `build.ps1 -Test`: 24/24 grupos de mecânicas; 8/8 de apresentação, gestos, animação e ambientes.
- O novo grupo verifica os oito frames de corrida, congelamento na pausa, estados aéreos, prioridade do deslize sobre a mordida, gestos de espera, entrada, impacto, derrota e reinício.
- Smoke visual conserva as verificações de jogabilidade, loja, saves, objetivos, escudo e ambientes recorrentes. Acrescenta capturas de saudação (`39`), impacto (`40`), queda (`41`), piscada (`42`) e ajuste do boné (`43`).
- Canal alpha e dimensões dos atlas inspecionados por `scripts/inspect-character-atlases.ps1`, sem edição dos pixels. Os recortes resultantes ficam em `CharacterAtlas.h`.
- Logs: `output/animation-tests.log`, `output/animation-visual.log`, `output/animation-visual-final.log`, `output/animation-visual-release.log`, `output/animation-android-final.log`.

Artes e prompts: `visual/Content/art/animation-prompts.json`. O atlas original fica preservado como referência. APK: `output/JacaRun-0.9.2-debug.apk`.

Não houve teste físico Android nesta rodada. Fluidez, memória e sensação de controle no aparelho ainda precisam de avaliação humana. As capturas verificam composição e poses, mas não substituem essa avaliação.
