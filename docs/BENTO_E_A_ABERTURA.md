# Bento, o Barão do Brejo — abertura 0.9.6

Bento é grande, vaidoso e protetor. Seu chapéu de palha gasto, lenço azul-petróleo, broche de peixe e dente lascado dão identidade ao rival. Ele não persegue Jaca por maldade: Jaca confunde Dourado, seu peixe de estimação, com comida.

## Roteiro — 12,5 segundos

1. **Bento e seu tesouro.** Aproximação lenta no hipopótamo orgulhoso e no peixinho. “Todo grandão tem um ponto fraco.” / “O dele era Dourado. Seu peixe de estimação.”
2. **A ideia errada.** Jaca se aproxima furtivamente: “Só uma mordidinha...”
3. **O dono aparece.** Corte para Bento, boca aberta, respingos e leve tremor de câmera: “TIRA A MÃO DO MEU DOURADO!”
4. **A pior desculpa.** Close no Jaca alarmado: “Eu? Tava só... dando um cheiro!”
5. **A fuga.** Jaca salta em arco para a margem, olhando para trás. Bento avança: “VOLTA AQUI, BONÉ VERMELHO!” O lanche virou perseguição.

Dourado continua vivo e visível. A cena termina no menu e não se repete a cada corrida. O botão INTRO permite rever; PULAR, Enter e Esc encerram a apresentação. Efeitos sonoros respeitam SOM.

## Direção e arquivos

O cenário ocupa toda a tela, com recorte proporcional, movimentos discretos de câmera e texto sobre degradês transparentes. Os personagens usam poses ilustradas, sem o hipopótamo geométrico anterior.

Artes geradas com a ferramenta integrada **image_gen**, copiadas para o projeto sem editar seus pixels ou remover o alpha:

- [Bento: quatro poses](../visual/Content/art/bento-story-v1.png)
- [Jaca: quatro expressões e ações](../visual/Content/art/jaca-story-v1.png)
- [Cenário vertical do brejo](../visual/Content/art/story-creek-v1.png)
- [Prompts completos utilizados](../visual/Content/art/story-prompts.json)

O atlas de Jaca existente foi a referência de identidade. `scripts/inspect-story-atlases.ps1` apenas lê dimensões, transparência e limites de cada pose, produzindo `StoryAtlas.h`.

## Validação

- 24/24 grupos de mecânicas e 9/9 de apresentação e história passaram.
- Smoke cobre cinco cenas, carregamento das artes, conclusão sem avanço da corrida, reprodução pelo menu e pulo pelo botão real.
- Logs: `output/bento-visual.log` e `output/bento-android.log`.
- Teste físico Android de fluidez e áudio permanece pendente.

Validacao final: PASS em output/visual-smoke-20261111-134131/result.txt. Composicao inspecionada em proporcoes regular e alta; sombras finais sem faixas e personagens com recorte na linha da agua. Build Android final concluido em output/bento-android-final.log. Capturas finais: docs/images/jacarun-0.9.6-bento.png e docs/images/jacarun-0.9.6-fuga.png.
