# JacaRun — protótipo visual 0.6

O aplicativo em `Source/` apresenta o mesmo `GameManager`, `Player`, `Level` e `Profile` da interface de terminal. Não existem regras de corrida separadas por plataforma. A cena cuida de desenho, input, menus e ciclo de vida; os diretórios nativos inicializam a aplicação e empacotam os recursos.

![Corrida no mangue](../docs/images/prototipo-corrida.png)

## Estado desta entrega

- Largura lógica de 480 pontos e altura adaptada ao aparelho: mangue em tela cheia, parallax, raízes, água, vegetação e personagem desenhados com `DrawNode`.
- Corrida contínua, pulo, deslize, obstáculos, coletáveis, power-ups e combos ligados ao núcleo.
- Menu, placar, pausa, resultado, reinício e loja de quatro aparências; acessórios equipados aparecem no jacaré.
- Corrida sem botões: gesto para cima pula, para baixo desliza, toque rápido com dois dedos pausa. Os gestos disparam durante o movimento; toque simples e movimento horizontal não pulam. Menus preservam suas ações e também aceitam gesto para cima para iniciar/retomar/reiniciar.
- Obstáculos e itens não coletados continuam passando atrás do jacaré até sair da tela. Coletáveis obtidos têm animação curta com partículas; a colisão/recompensa não é repetida.
- O aplicativo pausa ao ir para segundo plano e pede retomada explícita. Teclado continua disponível no computador.
- Save no diretório gravável da plataforma. Arquivo inválido é preservado; o jogo permite uma sessão sem gravar por cima dele.
- Bootstrap Windows e Android e arquivos de entrada iOS/macOS derivados do template oficial.

**Arte, ícones nativos e balanceamento são provisórios.** Não há áudio, anúncios, consentimento, telemetria, tutorial guiado, backend ou assinatura de produção. O empacotamento ainda inclui dependências padrão do módulo Java do Axmol; auditar e reduzir o inventário antes da publicação.

## Windows

Instale Git e Visual Studio com C++ desktop, SDK Windows e CMake. Na raiz:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build-visual.ps1 -Run
powershell -NoProfile -ExecutionPolicy Bypass -File .\build-visual.ps1 -Smoke
powershell -NoProfile -ExecutionPolicy Bypass -File .\build-visual.ps1 -Smoke -Aspect Tall
```

O setup não altera PATH nem AX_ROOT permanentemente. O primeiro build precisa de rede para baixar a engine e dependências. A política de scripts é definida apenas para o processo. Builds posteriores podem usar `-SkipSetup`. `-Configuration Release` compila uma versão otimizada, mas não significa aprovação para distribuição.

O teste de interface é exclusivo de Debug e usa `JACARUN_VISUAL_SMOKE`, definido automaticamente pelo script, com perfil isolado. Ele verifica:

1. Menu renderizado.
2. Gesto para cima e passagem por raiz.
3. Gesto para baixo e passagem sob galho.
4. Coleta de moeda e alimento.
5. Pausa com dois dedos e por ciclo de vida sem avançar a distância, e retomada por gesto.
6. Colisão e game-over.
7. Save/releitura e crédito único de moedas.
8. Reinício por gesto, preservando o saldo e zerando a distância.
9. Raiz ultrapassada e moeda perdida ainda visíveis atrás do jacaré.

O teste captura menu, salto, objetos ultrapassados, pausa e resultado, e grava `result.txt` em `output/visual-smoke-DATA-HORA`. `-Aspect Tall` usa uma janela 360 × 788, com a mesma proporção dos quadros extraídos do vídeo Android fornecido. Não substitui testes manuais de jogabilidade, multi-toque, recortes de tela e aparelhos físicos.

Para testar o núcleo sem baixar a engine:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

Execute em um Developer PowerShell, com CMake no PATH. O projeto raiz também gera `jacarun_cli`. Os 18 grupos do núcleo incluem 100 corridas longas e 100 percursos com verificação de progressão e espaçamento; três grupos adicionais testam gestos, permanência dos objetos, coleta animada, descarte fora da tela e eventos sem duplicação.

## Android de desenvolvimento

Ferramentas desta prova: JDK 17+ (validado com JBR 21), SDK/platform 36, build-tools 36.0.0, NDK r27c (27.2.12479018), CMake 4.2+ e Ninja 1.10+. O projeto usa Gradle 9.2.1, AGP 8.11.1, API mínima 23 e apenas `arm64-v8a`.

Na raiz:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build-android.ps1
```

O script procura as instalações convencionais. É possível informar `-AndroidSdk`, `-AndroidNdk` e `-JavaHome`. As dependências Gradle ficam em `.deps/gradle`. O helper cria uma vista local dos componentes já instalados em `.deps/android-sdk` e `local.properties`, ambos ignorados pelo Git. Ele não instala SDKs nem aceita licenças; componentes ausentes devem ser preparados no Android Studio.

Saída: `visual/proj.android/app/build/outputs/apk/debug/JacaRun-debug.apk`. O helper copia para `output/JacaRun-debug.apk` e `output/JacaRun-0.6.0-debug.apk` (versão obtida do metadata do build). É um APK de desenvolvimento; não é AAB de loja e não tem assinatura de produção. O segundo vídeo mostra a 0.3; a 0.6 precisa de reteste físico. Veja o [registro de validação](../docs/VALIDACAO_VISUAL.md).

Para instalar em um Android próprio conectado e autorizado para depuração:

```powershell
adb devices
adb install -r .\visual\proj.android\app\build\outputs\apk\debug\JacaRun-debug.apk
```

A instalação/abertura em aparelho precisa ser registrada separadamente da compilação. Não usar `-r` sobre uma versão com progresso importante sem backup/teste de compatibilidade.

## iOS

Os arquivos `proj.ios_mac/` são a base do template, com orientação vertical. Não foi feita compilação ou instalação iOS neste ambiente Windows. Em um Mac será necessário preparar a mesma revisão Axmol, ferramentas Apple e gerar o projeto Xcode pelo build oficial da engine. Validar em iPhone, configurar equipe/assinatura e revisar identificador antes do TestFlight.

O identificador provisório é `com.mgreboucas.jacarun` nas duas plataformas. Não foi verificada sua disponibilidade nas lojas.

## Comportamento e limites

- A simulação limita um frame a 100 ms para evitar saltos após travamentos; em dispositivo lento isso reduz o ritmo. Medir desempenho e ajustar antes de produção.
- O layout usa `FIXED_WIDTH`, altura real e margens da safe area no HUD. O cenário cobre também as regiões antes reservadas aos controles. Testar notch, navegação por gestos e telas físicas com proporções diferentes.
- `RunPresentation` preserva os objetos por ID após a colisão pontual; apenas coletáveis confirmados animam e somem antes da borda. `GestureInput` é independente da engine e possui testes próprios.
- Colisões continuam sendo as regras pontuais do protótipo. O desenho usa o ponto de contato do focinho como referência; ajustar sensação de contato e caixas visuais após testes humanos.
- Moedas de uma corrida são creditadas ao perder ou encerrar pelo menu. Segundo plano pausa e salva o perfil já consolidado, mas não restaura uma corrida se o sistema encerrar o processo.
- O save visual é separado do terminal. Migração, recuperação guiada, nuvem e proteção contra adulteração ainda não foram implementadas.
- A suíte visual percorre um cenário determinista. Ela não atesta retenção, duração de bateria, sessões de 15 minutos ou prontidão para publicidade.
- Houve aviso de bibliotecas de runtime diferentes no primeiro link Debug do Windows e avisos internos do Axmol/NDK no Android. As verificações de execução e a auditoria de dependências continuam obrigatórias para produção.

## Dependências e licenças

| Componente | Revisão/origem | Licença |
| --- | --- | --- |
| Axmol + template | [v2.11.4](https://github.com/axmolengine/axmol/tree/b14941e6f50a0ce12489bd8f57041e093fb58819) | [MIT](licenses/Axmol-MIT.txt) |
| Compilador de shaders | [axslcc 1.14.0](https://github.com/axmolengine/axslcc/releases/tag/v1.14.0) | Ferramenta local da engine; não incluída no jogo |
| Kanit Regular/SemiBold | [Google Fonts / Kanit](https://github.com/google/fonts/tree/main/ofl/kanit) | [SIL OFL 1.1](Content/fonts/OFL.txt) |
| Gradle wrapper | 9.2.1, template oficial | [Apache 2.0 e avisos](licenses/Gradle-Apache-2.0.txt) |
| Cenário e jacaré | Geometria em `MainScene.cpp` | Código deste projeto; arte provisória |

O bootstrap valida o commit da engine e o SHA256 do pacote Windows do axslcc. As fontes são versionadas no repositório. Os ícones e a launch screen originais do template devem ser substituídos pela identidade final. As dependências transitivas da engine têm licenças próprias em `.deps/axmol/3rdparty`; concluir inventário/avisos de distribuição antes de publicar.

Progresso e critérios de conclusão: [cronograma](../CRONOGRAMA_PRODUCAO.md).

### Chave Debug e atualizações no aparelho

A 0.4.0 foi gerada com um certificado diferente da 0.3, impedindo a atualização por cima. A 0.4.1 restaura o certificado original. O helper preserva a chave em `.deps/signing/debug.keystore` (ignorada pelo Git), inicialmente copiada de `%USERPROFILE%/.android/debug.keystore`. O Gradle usa esse caminho explicitamente, sem depender do usuário da execução. Antes de copiar o APK para `output`, o helper verifica a assinatura e exige a impressão SHA-256 do certificado original da 0.3.

Em outro computador, restaure a mesma chave em `.deps/signing/debug.keystore`; não gere uma substituta para atualizar instalações existentes. Guarde uma cópia privada da chave fora do repositório. Essa chave é apenas de desenvolvimento, não de produção.
