# Ambientes recorrentes — 0.9.1

- Seletor removido do menu e preferência antiga ignorada.
- Toda partida começa no mangue. Aos 500 m entra a vila; aos 1.000 m, a floresta; aos 1.500 m, o mangue. O ciclo continua indefinidamente.
- Fundo e piso se misturam gradualmente nos primeiros 40 m do novo trecho, com interpolação suave. Os fundos são carregados antes da corrida.
- A distância da simulação controla o visual. Pausa e fim da partida congelam a transição; nova partida reinicia o ciclo. O menu sempre mostra o mangue.
- Colisões, geração de obstáculos, recompensas e progresso permanecem independentes dos ambientes.

## Verificação

`build.ps1 -Test`: 24/24 grupos de mecânicas e 7/7 de apresentação. O novo grupo verifica quatro voltas completas, limites de cada trecho, metade/fim da transição, pausa, retomada e reinício real da partida.

Smoke visual ampliado: uma única corrida sem obstáculos avança pelos três ambientes, mistura floresta/mangue, completa o retorno e reinicia. Capturas `35-village-run`, `36-forest-run`, `37-mangrove-return` e `38-environment-transition`.

Logs locais: `output/cycle-tests.log`, `output/cycle-visual.log`, `output/cycle-visual-final.log` e `output/cycle-android-final.log`. Android físico ainda requer teste de fluidez e toque. APK: `output/JacaRun-0.9.1-debug.apk`.
