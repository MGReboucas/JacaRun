# Jaca vestido — 0.9.23

Cada visual da loja usa o personagem completo pintado com o acessório integrado: perspectiva, sombras de contato e sobreposição anatômica fazem parte da própria arte. O renderizador não monta chapéus ou óculos separados sobre uma base sem acessórios.

- `jaca-01.png` até `jaca-15.png`: 24 poses por visual, em quatro colunas e seis linhas. Linhas 1–2: descanso/entrada; 3–4: corrida; 5–6: salto, deslize, mordida, mastigação e derrota.
- `jaca-05-run.png`: ciclo de corrida dedicado da coroa, em quatro colunas e duas linhas. Substitui as poses de corrida da folha principal.
- `jaca-09-idle.png` e `jaca-09-run.png`: descanso e corrida dedicados do explorador, com o mesmo tamanho e posi??es dos atlas originais.
- O visual 0 continua usando os três atlas originais do boné vermelho.
- `layout.json` registra a escala do corpo de cada folha. A altura do chapéu não determina o tamanho do Jaca.
- `prompts.json` registra os prompts usados com a ferramenta integrada ImageGen e as correções de transparência. Nenhuma montagem de acessórios isolados foi usada para produzir estas versões.

Execute `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/inspect-skin-atlases.ps1` na raiz após alterar uma folha. O script lê o alpha, localiza as silhuetas conectadas e gera `visual/Source/DressedJacaAtlas.h`, sem modificar os PNGs. A detecção permite que caudas longas atravessem a margem entre células sem serem cortadas ou incluídas na pose vizinha.

`CharacterSkin.h` seleciona a arte completa para menu, loja, provador, tutorial, resultado e corrida. A introdução narrativa pré-ilustrada mantém o figurino original. As compras e os identificadores do catálogo não mudaram.

O smoke visual percorre o provador e renderiza todas as 384 poses, com capturas `68-skin-N-all-poses.png`, além dos quadros de corrida, salto e deslize. Os testes de apresentação verificam a resolução de cada visual/estado/quadro e os limites dos índices.
