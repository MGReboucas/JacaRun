# Dourado participa da cena — 0.9.8

O movimento independente do peixe foi removido. Dourado agora segue ações e pontos dos personagens:

- **Apresentação:** repousa nas mãos de Bento e acompanha o movimento do corpo, sem órbita ou rotação automática.
- **Aproximação:** permanece na mão estendida; ao perceber Jaca, mergulha naquele ponto da água.
- **Mordida:** salta da água e permanece preso ao focinho durante o recuo de Jaca. O ponto acompanha escala, espelhamento, rotação e câmera.
- **Retorno:** solta o focinho, volta ao mesmo ponto da água e desaparece sob a superfície, com uma ondulação localizada.
- **Bronca:** aparece parcialmente submerso junto de Bento e se recolhe. A legenda confirma que voltou ao dono.
- **Desculpa e perseguição:** fica fora do enquadramento. Não flutua junto aos personagens nem acompanha a fuga pelo ar.

A imagem de derrota enviada pelo usuário e a trilha da 0.9.7 foram preservadas.

## Verificações

- 24/24 grupos de mecânicas e 10/10 de apresentação passaram.
- O novo grupo verifica mãos estáveis, fases de mergulho, contato durante a mordida, acompanhamento do focinho em movimento, retorno e ausência durante a perseguição.
- Smoke verifica a posição renderizada do peixe em relação ao focinho transformado e que ele não aparece no salto final.
- Capturas adicionais: `54-story-fish-dive.png` e `55-story-fish-return.png`.
- Logs: `output/dourado-visual.log` e `output/dourado-android.log`.
- Teste de percepção do movimento em celular ainda necessário.

Smoke em tela alta PASS: output/visual-smoke-20261111-141334/result.txt. Inspecionadas as capturas de maos, mergulho, contato no focinho, retorno submerso e ausencia na fuga. APK Android gerado com sucesso.
Smoke regular tambem PASS: output/visual-smoke-dourado-regular/result.txt; contato no focinho inspecionado nessa proporcao.
