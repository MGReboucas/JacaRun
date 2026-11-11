# Derrota e abertura com mais vida — 0.9.7

## Imagem de derrota

A imagem enviada pelo usuário foi salva em `visual/Content/art/jaca-defeat-user.png`, sem recriação, recorte ou edição de pixels. Comparação binária confirmou que o PNG é idêntico ao anexo. Dimensões: 232 × 199; apresentação proporcional acima de “Boa, Jaca!”. A carinha da loja permanece independente.

SHA-256: `93e3087f1d333ded1caa608c24bd0c608bacbff274c8bc8a85a9aca710139f20`.

## Abertura: seis momentos, 15,2 segundos

1. **0–2,4 s:** Bento cuida de Dourado: “Fica perto, Dourado. Tem folgado na água.”
2. **2,4–5 s:** Jaca se aproxima: “Só um lanchinho. Ninguém vai perceber.”
3. **5–6,9 s:** Dourado salta e dá uma mordida no focinho. Jaca recua: “AI! O LANCHE ME MORDEU!”
4. **6,9–9,5 s:** Bento reage com respingos e tremor curto: “É MEU AMIGO, SEU FOMINHA!”
5. **9,5–12,1 s:** Pausa constrangedora, pose de desculpa: “Eu tava só... dando um beijinho!”
6. **12,1–15,2 s:** Preparação do salto, arco, aterrissagem e corrida com oito frames para fora da tela. “CORRE, JACA!”

O peixe continua vivo. A gag é cartunesca, sem ferimentos. Os tempos variam por cena; notas entram depois das reações e o indicador acompanha a passagem do tempo.

## Áudio

`scripts/generate-story-audio.js` gera `visual/Content/audio/story-score.wav`: trilha original instrumental com água, notas dedilhadas, estalo cômico, acorde grave, grilos na pausa, respingo e aceleração final. Não utiliza vozes, gravações ou amostras externas.

PCM mono a 22.050 Hz, 15,2 s, pico limitado a 0,72. SOM controla a reprodução. Pular e concluir interrompem a trilha; segundo plano interrompe o áudio e, ao retornar, a reprodução busca o tempo da cena. Correção de diferença acima de 250 ms evita acumular dessincronização em aparelhos lentos.

## Validação

- 24/24 grupos de mecânicas e 9/9 de apresentação e história passaram.
- Smoke verifica a imagem carregada, as seis cenas, transição do salto para o atlas de corrida, retorno ao menu, reprodução e pulo pelo botão.
- Capturas: `04-game-over.png`, `52-story-fish-bite.png`, `53-story-run.png` e demais cenas.
- Logs finais: `output/story-life-visual-final.log`, `output/story-life-android-final.log`.
- Fluidez, latência e volume precisam de avaliação humana no celular.

Smoke final PASS: output/visual-smoke-20261111-140349/result.txt. Versao alta tambem passou em output/visual-smoke-20261111-140205/result.txt. Capturas da derrota e fuga inspecionadas nas duas proporcoes. Build Android final concluido com sucesso.
