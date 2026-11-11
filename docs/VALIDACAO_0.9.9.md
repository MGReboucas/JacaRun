# Abertura 0.9.9 — Dourado participa da atuação

A revisão substitui o sprite independente do peixe por contato desenhado: mãos envolvendo Dourado, boca no focinho e proteção junto ao peito. A câmera transforma a composição inteira, mantendo essas relações. A cena compartilhada mostra a aproximação do Jaca; a perseguição usa quatro poses do Bento carregando o amigo.

## Roteiro em 15,2 segundos

1. Bento cuida de Dourado: “Fica perto, Dourado. Tem folgado na água.”
2. Jaca estica a mão: “Só um lanchinho. Ninguém vai perceber.”
3. Antecipação, mordida, reação e soltura: “AI! O LANCHE ME MORDEU!”
4. Bento protege o amigo e aponta: “É MEU AMIGO, SEU FOMINHA!”
5. Jaca improvisa: “Eu tava só... dando um beijinho!”
6. Jaca salta para a margem e corre; Bento persegue segurando Dourado.

A batida da mordida ocorre aos 5,15 s, acompanhando a troca de pose. A introdução continua em tela inteira, com PULAR e INTRO, sem iniciar a corrida automaticamente.

## Artes e origem

Geradas com image_gen integrado, preservadas em PNG transparente sem manipulação dos pixels:

- [Interações compartilhadas](../visual/Content/art/story-interactions-v2.png).
- [Sequência da mordida](../visual/Content/art/jaca-bite-sequence.png).
- [Bento protegendo Dourado na corrida](../visual/Content/art/bento-guardian-run.png).
- [Prompts completos](../visual/Content/art/story-contact-prompts.json).

O script inspect-story-contact-atlases.ps1 lê limites alpha e gera StoryContactAtlas.h; não altera as imagens. A imagem de derrota enviada pelo usuário permanece integral.

## Verificação

24/24 grupos de mecânicas e 10/10 grupos de apresentação passaram. Capturas do executável foram inspecionadas e os enquadramentos de mordida e bronca receberam mais espaço. O smoke verifica poses integradas, proteção na fuga, salto seguido de corrida, replay e pulo da introdução. Logs finais: output/tests-0.9.9.log, output/build-0.9.9-tall-final.log e output/build-android-0.9.9.log.

O teste em aparelho Android físico permanece pendente.

Smoke final: PASS nas proporcoes Tall e Regular. Evidencias: output/visual-smoke-20261111-143243 e output/visual-smoke-0.9.9-regular. Android: BUILD SUCCESSFUL, APK 0.9.9 com assinatura verificada pelo script.

Capturas: [mordida](images/jacarun-0.9.9-mordida.png) e [protecao](images/jacarun-0.9.9-protecao.png).
