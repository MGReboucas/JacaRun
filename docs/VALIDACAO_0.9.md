# Validação visual 0.9.0 — 10/11/2026

Implementação baseada na referência fornecida pelo usuário: Jaca de boné e bandana, mangue, vila de pescadores e floresta tropical. Esta entrega altera a apresentação; os três ambientes compartilham o percurso e as regras existentes.

## Verificações

- `build.ps1 -Test`: 24/24 grupos de mecânicas e 6/6 grupos de apresentação, gestos e animação passaram.
- Windows Debug compilado e executado. Smoke em 432 × 810 e 360 × 788: PASS.
- Percurso, coleta, pausa, reinício, salvamento, objetivos, loja, encadeamento de gestos e escudo exercitados pelo smoke existente.
- Smoke ampliado: seleção da vila e floresta, retorno circular ao mangue e carregamento de todas as texturas de gameplay. Capturas de ambiente separadas das mudanças de UI para respeitar a captura assíncrona da engine.
- Capturas inspecionadas: menu, corrida, deslize, loja e ambientes. Proporção do personagem, apoio no piso e altura do deslize ajustados.
- Build Android 0.9.0 (versionCode 21) concluído; assinatura verificada pelo script existente. Os sete PNGs foram encontrados dentro do APK.

Evidências locais:

- `output/art-tests.log`
- `output/art-tall.log`
- `output/visual-smoke-20261110-191539/result.txt` (tela alongada)
- `output/visual-smoke-final-20261110-191626/result.txt` (tela regular, código final)
- `output/art-android.log`
- `output/art-android-final.log`

Prévia permanente em `docs/images/jacarun-0.9-*.png`. Artes e prompts em `visual/Content/art/README.md`.

## Limites

Não houve instalação nem teste em aparelho físico nesta rodada. Toque real, fluidez e memória no Android precisam de avaliação no celular. iOS não foi compilado neste ambiente. A loja mantém os acessórios como sobreposições sobre o novo atlas; os ambientes ainda não têm mecânicas, alimentos ou regras de desbloqueio próprios. A referência enviada contém três ambientes; um quarto ainda não foi definido.
