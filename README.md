# JACARUN

**Corra. Coma. Compita.**

Consulte o [cronograma de produção e monetização](CRONOGRAMA_PRODUCAO.md) para as etapas de Android, iOS, publicação nas lojas e integração com AdMob.

JacaRun é um **endless runner 2D em desenvolvimento**, pensado para celulares em orientação vertical. Um jacaré corre pelo mangue, supera obstáculos, captura alimentos e coleta moedas para personalizar sua aparência.

Este repositório contém um **protótipo visual em C++/Axmol**, com corrida em tempo real, mangue em tela cheia, controles por gestos, cenário provisório, loja, pausa, save e reinício. A interface de terminal continua disponível e compartilha as mesmas regras. A versão 0.7 reformula a loja com sete visuais e três melhorias permanentes, preços de longo prazo e peixes difíceis que aceleram a pontuação. Os vídeos Android orientaram os ajustes; esta revisão ainda precisa de reteste físico.

![Protótipo visual do JacaRun](docs/images/prototipo-menu.png)

## Jogar a versão visual no Windows

Requisitos: Git e Visual Studio 2022/2026 com desenvolvimento desktop C++ e CMake. O primeiro build baixa o Axmol **2.11.4**, fixado por commit, e suas dependências.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build-visual.ps1 -Run
```

- **Pular:** arraste para cima em qualquer ponto da corrida.
- **Deslizar:** arraste para baixo. O gesto responde durante o movimento do dedo.
- **Pausar:** toque rápido com dois dedos. Para retomar, arraste para cima na tela de pausa.
- **Computador:** Espaço/seta para cima pula; seta para baixo desliza; P/Esc pausa. Enter inicia/reinicia ou retoma.
- **Loja:** SEU JACA no menu; acessórios usam apenas moedas coletadas no jogo.

O executável fica em `visual/build-win32/bin/JacaRun/Debug/JacaRun.exe`. No Windows, o save visual fica em `%LOCALAPPDATA%/JacaRun/jacarun.save`; ele é separado do save do terminal. Sair para o menu ou perder contabiliza a corrida. Ir para segundo plano pausa; uma corrida interrompida pelo encerramento do processo não é restaurada.

Para reproduzir a validação automática da interface:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build-visual.ps1 -Smoke
```

O teste usa perfil isolado, envia eventos de gesto e verifica coleta, permanência dos objetos ultrapassados, pausa com dois dedos, save e reinício. Use também `-Aspect Tall` para testar a proporção 360 × 788 do vídeo de referência. As capturas e o relatório ficam em `output/visual-smoke-DATA-HORA/`. Consulte [o guia técnico visual](visual/README.md) para Android, iOS, limitações e licenças.

**APK atual:** `output/JacaRun-0.7.0-debug.apk`. Transfira ao celular e instale como atualização da versão anterior; o identificador foi preservado e o save antigo é migrado automaticamente. Não é uma versão de loja.

### Ritmo da corrida

O percurso começa com raízes e galhos. Duplas aparecem após 180 m; troncos caídos, pedras e cipós entram após 300 m; triplas após 550 m. Troncos/pedras exigem pulo e cipós exigem deslize. A pressão por pontuação começa aos 20 mil. Aos 50 mil, as sequências chegam a seis obstáculos; aos 75 mil, sete; aos 100 mil, oito. A geração usa a pontuação ao preparar o próximo trecho, sem mover obstáculos que já existem.

A velocidade base cresce com a distância até 24 m/s. Não há bônus de velocidade por pontuação antes de 20 mil: a partir daí ele aumenta continuamente, levando a corrida a 36 m/s aos 35 mil, 48 m/s aos 50 mil, 54 m/s aos 75 mil e 60 m/s aos 100 mil, quando a aceleração base está completa. No modo Domínio, a separação mínima é 30 m: um obstáculo a cada 0,5 s no limite. Pulo e deslize continuam cobrindo cerca de 20,7/21,6 m; moedas mantêm o arco e a separação mínima de 5,5 m. O HUD sinaliza as faixas. Cada tentativa reinicia suavemente; esses valores são experimentais e precisam de teste humano.

## Decisões confirmadas no GDD

| Aspecto | Definição |
| --- | --- |
| Plataforma e orientação | Mobile, vertical |
| Gênero e visual | Endless runner, primeira versão em 2D |
| Linguagem | C++ |
| Personagem e cenário inicial | Jacaré no mangue |
| Movimento | Corrida automática |
| Pontuação | Pontos por desempenho, com bônus por alimentos |
| Moedas | Coleta durante a corrida e uso na personalização |
| Loja | Acessórios |

A identidade proposta é brasileira, tropical, cartoon e bem-humorada. O GDD também descreve competição semanal e progressão de longo prazo.

**Os valores, preços, tipos de power-ups e regras detalhadas abaixo são decisões experimentais deste protótipo.** Eles não substituem a aprovação e o balanceamento das seções ainda abertas no GDD.

## Compilar e jogar no terminal

Para a interface de terminal, é necessário um compilador com suporte a **C++17**. O núcleo não depende da engine; no Windows, o salvamento usa também a API do sistema para substituir o arquivo com segurança.

Na raiz do projeto, usando PowerShell com `g++` disponível no `PATH`:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1
.\output\jacarun.exe
```

A opção de execução de scripts vale apenas para esse processo do PowerShell. Como alternativa, compile diretamente:

```powershell
g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude main.cpp src/GameManager.cpp src/Player.cpp src/Level.cpp src/Profile.cpp -o output/jacarun.exe
.\output\jacarun.exe
```

O executável atualizado é **`output/jacarun.exe`**. O antigo `output/main.exe`, já versionado no projeto, não é atualizado pelo script.

Para assistir a uma corrida automática de demonstração:

```powershell
.\output\jacarun.exe --demo --seed 42
```

A demonstração não lê nem grava o progresso do jogador. A seed permite repetir o mesmo percurso.

## Controles do terminal

Digite o comando e pressione Enter.

| Onde | Comando | Ação |
| --- | --- | --- |
| Menu ou fim de partida | `jogar` ou `j` | Inicia uma nova corrida |
| Menu ou fim de partida | `loja` | Mostra saldo, acessórios e IDs |
| Menu ou fim de partida | `comprar ID` | Compra um acessório |
| Menu ou fim de partida | `equipar ID` | Equipa um acessório adquirido |
| Corrida | Enter ou `correr` | Avança a corrida por 0,2 segundo |
| Corrida | `pular` ou `p` | Tenta pular e avança 0,2 segundo |
| Corrida | `agachar` ou `a` | Tenta deslizar e avança 0,2 segundo |
| Corrida ou pausa | `pausa` | Pausa ou retoma a partida |
| Qualquer tela | `menu` | Encerra a corrida, contabiliza recompensas e volta ao menu |
| Qualquer tela | `ajuda` | Exibe os comandos |
| Qualquer tela | `sair` | Encerra a corrida, salva e sai |

Pulo e deslize precisam aguardar o fim da ação anterior. Tentar uma ação indisponível ainda consome o turno. Comandos desconhecidos não avançam o tempo.

O painel informa distância, velocidade, pontuação, moedas, combo, altura do personagem, efeitos ativos e os próximos objetos. Use o tempo estimado até o obstáculo para reagir: iniciar um pulo ou deslize cerca de **0,3 segundo antes** da chegada é uma referência útil com o balanceamento atual.

## Mecânicas implementadas

### Corrida e dificuldade

A distância resulta da velocidade multiplicada pelo tempo simulado. A velocidade começa em **10 m/s**, aumenta com distância e pontuação e tem limite de **60 m/s**.

O percurso é gerado continuamente à frente do personagem. Os encontros ficam mais próximos conforme a corrida avança, respeitando um espaçamento mínimo. Objetos já atravessados são removidos, mantendo o tamanho do cenário em memória limitado.

A simulação divide cada atualização em passos de até **1/120 segundo**. As colisões consideram os objetos cruzados no deslocamento, inclusive quando o personagem ultrapassa a posição de um obstáculo entre duas atualizações.

### Pulo, deslize e obstáculos

- **Raiz:** exige altura de pelo menos 0,7 unidade no momento do encontro.
- **Galho:** exige estar no chão e deslizando.
- **Pulo:** só começa no chão, fora de um deslize; não há pulo duplo.
- **Deslize:** cobre 21,6 metros; dura cerca de 2,16 s a 10 m/s e 0,9 s a 24 m/s. Não pode começar no ar nem ser renovado enquanto está ativo.
- **Colisão:** encerra a corrida, exceto quando um escudo ativo absorve a batida.

Pulo e deslize avançam conforme a distância percorrida. O salto cobre aproximadamente 20,7 m, com a mesma altura e duração maior na corrida lenta. Moedas formam um arco correspondente a um salto iniciado 6 m antes da raiz; os coletáveis ficam separados por pelo menos 5,5 m. A pausa congela movimento, combo, efeitos e geração do percurso.

### Alimentos, combos e recordes

| Alimento | Pontos base | Posição no percurso |
| --- | ---: | --- |
| Caranguejo | 20 | Baixo |
| Peixe | 30 | No alto do salto; ativa pontos de distância ×2 por 6 s |
| Peixe raro | 100 | No alto do salto, coleta mais precisa; distância ×3 por 10 s |

A coleta considera a altura do personagem. Peixes exigem precisão vertical: tolerância de 0,22 unidade no comum e 0,10 no raro, contra 0,85 nos demais coletáveis. O peixe ocupa uma posição própria no arco, sem sobrepor as três moedas. Ímã não coleta peixe. Alimentos fora do alcance são perdidos.

Cada alimento capturado aumenta a sequência do combo. A cada três alimentos, o multiplicador sobe um nível, até **x5**. Ele vale para os pontos dos alimentos. A sequência termina ao perder um alimento, ficar oito segundos sem capturar outro (dez com a melhoria Fôlego do combo) ou sofrer uma colisão absorvida pelo escudo.

**Pontuação = pontos de distância acumulados (×1, ×2 ou ×3 durante o frenesi) + pontos dos alimentos com multiplicador de combo.** O bônus do peixe não multiplica moedas nem altera diretamente a física. Ele ajuda a atingir as faixas difíceis de pontuação mais cedo. Novos peixes renovam a duração sem somar segundos e preservam o maior multiplicador ainda ativo. Pausa congela o efeito; colisão absorvida pelo escudo e nova corrida encerram o frenesi.

Os recordes de pontuação e distância são atualizados ao encerrar a corrida.

### Moedas, loja e personalização

Moedas coletadas ficam no saldo da corrida e são creditadas na carteira quando ela termina, inclusive ao voltar ao menu ou sair. Cada corrida credita suas recompensas apenas uma vez.

| ID | Acessório | Preço |
| --- | --- | ---: |
| 0 | Jaca original | Gratuito, já adquirido |
| 1 | Boné do mangue | 2.000 moedas |
| 2 | Óculos tropicais | 5.000 moedas |
| 3 | Chapéu de pescador | 9.000 moedas |
| 4 | Bandana vermelha | 15.000 moedas |
| 5 | Coroa do mangue | 30.000 moedas |
| 6 | Capacete lunar | 50.000 moedas |
| 7 | Fôlego do combo: +2 s para manter a sequência | 12.000 moedas |
| 8 | Ímã duradouro: +4 s em cada ímã coletado | 20.000 moedas |
| 9 | Frenesi prolongado: +3 s no bônus dos peixes | 35.000 moedas |

A loja tem abas de visuais e melhorias, páginas, descrição de efeitos, saldo e feedback de compra. Visuais 0–6 são equipáveis e aparecem no jacaré. Melhorias 7–9 são permanentes e ficam ativas automaticamente: não precisam ser equipadas nem recompradas. O jogo bloqueia compras repetidas e saldo insuficiente. Não há compra com dinheiro real.

Os preços são metas experimentais de esforço: em 100 corridas automatizadas de 200 s, sem melhorias, a média foi 321,73 moedas (312–336). Nesse desempenho, o boné exige aproximadamente 7 corridas completas; o capacete, 156. Isso não é previsão de desempenho humano: derrotas e coletas perdidas aumentam o tempo. Revisar retenção e preços após testes reais, sem retirar compras já conquistadas.

### Power-ups experimentais

- **Escudo:** protege de uma colisão e é consumido ao absorvê-la; expira após 12 segundos se não for utilizado.
- **Ímã:** durante 10 segundos (14 com melhoria), permite coletar moedas em qualquer altura quando elas cruzam o personagem.

Os efeitos são encontrados no percurso. Recolher o mesmo tipo renova sua duração. Ambos são removidos ao iniciar uma nova corrida.

### Experiência e níveis

Cada corrida concede:

- 1 XP a cada 10 metros completos;
- 3 XP por alimento capturado;
- 5 XP por obstáculo superado sem depender de escudo.

O jogador ganha um nível a cada **200 XP**. Os níveis registram a progressão local; ainda não desbloqueiam biomas ou missões.

## Salvamento

O perfil é salvo automaticamente ao encerrar a corrida, comprar ou equipar um acessório, voltar ao menu e sair. Ele inclui carteira, XP, recordes, visuais, melhorias e equipamento atual. A versão 0.7 lê saves antigos e grava o formato v2, preservando moedas e compras anteriores, sem cobrar diferenças de preço. Versões antigas do app não leem o formato novo.

O caminho padrão é `output/jacarun.save`, relativo à pasta de execução. Os arquivos de progresso e os novos executáveis são ignorados pelo Git.

```powershell
# Jogar sem ler nem gravar progresso
.\output\jacarun.exe --no-save

# Usar outro perfil e um percurso reproduzível
.\output\jacarun.exe --save output/outro-perfil.save --seed 42
```

A gravação usa um arquivo temporário antes de substituir o save anterior. Arquivos inválidos ou incompatíveis interrompem o carregamento e são preservados; nesse caso, é possível jogar com `--no-save` ou escolher outro caminho. Erros de gravação são exibidos no terminal.

A partida em andamento não é restaurada após fechar o programa. O salvamento é local e não possui sincronização online.

## Testes

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1 -Test
```

A suíte verifica física, restrições das ações, colisões, consumo único de objetos, pontuação, combos, coleta por altura, power-ups, pausa, reinício, recompensas, compras e salvamento. Também executa **100 percursos com seeds diferentes**, alcançando a velocidade máxima, para verificar que as sequências geradas podem ser atravessadas.

## Estrutura do projeto

```text
.
├── main.cpp                  # Interface do terminal, comandos e demonstracao
├── build.ps1                 # Compilacao do jogo e testes
├── build-visual.ps1          # Protótipo Axmol no Windows e teste visual
├── build-android.ps1         # APK Android de desenvolvimento
├── CMakeLists.txt            # Núcleo, terminal e testes sem a engine
├── visual/                   # Cena Axmol e adaptadores de plataforma
├── include/
│   ├── Balance.h             # Parametros de fisica, dificuldade e recompensas
│   ├── GameManager.h         # Estados e regras da partida
│   ├── Level.h               # Objetos e geracao do percurso
│   ├── Player.h              # Pulo e deslize
│   └── Profile.h             # Progresso e acessorios
├── src/
│   ├── GameManager.cpp       # Colisoes, coleta, combos e recompensas
│   ├── Level.cpp             # Geracao procedural e objetos atravessados
│   ├── Player.cpp            # Fisica por tempo decorrido
│   └── Profile.cpp           # Catalogo, compras e persistencia
├── tests/
│   └── mechanics_tests.cpp   # Testes automatizados
└── output/                   # Executaveis e saves locais
```

Os principais parâmetros estão em `include/Balance.h`; os acessórios e seus preços estão no catálogo de `src/Profile.cpp`.

## Etapas ainda abertas

O ciclo local do protótipo está implementado. A visão completa do GDD ainda depende de:

- Arte e animações finais, áudio, tutorial e acessibilidade.
- Validação da interface/toque em aparelhos Android/iOS e builds para as lojas.
- Ranking online semanal, contas, validação de pontuações e recompensas competitivas.
- Novos biomas, fases, missões, conquistas e regras de desbloqueio.
- Balanceamento final, monetização e fechamento do escopo da versão 1.0.

## Referência

Baseado no **JACARUN — Game Design Document, versão 0.1**, com status **Em planejamento**, e no código deste repositório. As decisões aprovadas estão registradas na seção **22. Decisões fechadas** do GDD. As regras experimentais documentadas acima servem para testar o jogo enquanto os detalhes de design são consolidados.
