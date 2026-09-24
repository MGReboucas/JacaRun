# JACARUN

**Corra. Coma. Compita.**

JacaRun é um jogo em desenvolvimento do gênero **endless runner 2D**, pensado para celulares em orientação vertical. O jogador acompanha um jacaré em uma corrida automática por ambientes inspirados na natureza brasileira, começando pelo mangue. O objetivo é chegar cada vez mais longe, superar obstáculos, coletar moedas e capturar alimentos para aumentar a pontuação.

O projeto utiliza **C++** e está na etapa de planejamento e prototipagem. Atualmente, este repositório contém uma simulação de lógica no terminal, com pulo, gravidade e evolução de pontuação e velocidade.

## Proposta do jogo

Partidas curtas, comandos simples e dificuldade crescente formam a base do JacaRun. A corrida não tem uma linha de chegada tradicional: o jogador tenta sobreviver, melhorar seu desempenho e iniciar uma nova tentativa após perder.

O mangue participa da identidade e da proposta de jogabilidade. O GDD apresenta raízes, troncos, galhos, peixes e caranguejos como exemplos de elementos que podem se transformar em obstáculos e oportunidades de pontuação. Esses exemplos ainda não constituem uma lista final de conteúdo.

A direção visual definida é cartoon 2D, tropical, colorida e bem-humorada, com um jacaré expressivo como personagem principal.

## Decisões confirmadas

As decisões registradas como fechadas no GDD são:

| Aspecto | Definição |
| --- | --- |
| Plataforma | Mobile |
| Orientação da tela | Vertical |
| Gênero | Endless runner |
| Visual da primeira versão | 2D |
| Linguagem | C++ |
| Personagem principal | Jacaré |
| Cenário inicial | Mangue |
| Movimento | Corrida automática |
| Pontuação | Sistema de pontos, com pontuação adicional por alimentos |
| Coletáveis | Moedas |
| Uso das moedas | Personalização do personagem |
| Loja | Acessórios |

Essas definições orientam o desenvolvimento. Nem todas estão implementadas no protótipo atual.

## Mecânicas do jogo

### Corrida e dificuldade

O jacaré corre automaticamente, enquanto o jogador reage aos desafios do percurso. A proposta do GDD é aumentar a velocidade e a dificuldade conforme a distância percorrida, exigindo mais precisão nas ações.

No código atual, a corrida é representada por atualizações no terminal. Durante o estado `PLAYING`, cada atualização acrescenta um ponto e aumenta a variável de velocidade em `0.01`, a partir do valor inicial `10.0`. Ainda não há deslocamento visual, geração de obstáculos ou cenário infinito.

### Pulo e gravidade

O protótipo já permite iniciar um pulo quando o personagem está no chão. A gravidade altera sua velocidade vertical a cada atualização até ele retornar à altura zero. Novos pulos são bloqueados enquanto está no ar.

Os parâmetros atuais são força de pulo `8.0` e gravidade `-2.0` por atualização. São valores do protótipo, sem ajuste por tempo decorrido (`deltaTime`), e não representam o balanceamento final.

O GDD prevê controles por toque e gesto, mas o mapeamento ainda está em aberto. Na simulação, o pulo acontece automaticamente no terceiro quadro, sem entrada do jogador.

### Obstáculos e fim de partida

A proposta é pular, abaixar e desviar para continuar correndo. Um erro que encerre a corrida leva ao fim da partida, seguido de uma nova tentativa.

O código já declara os estados `MENU`, `PLAYING` e `GAMEOVER`, além de um método `GameOver()`. Ainda não existem obstáculos, detecção de colisões, ação de abaixar ou telas para esses estados. A simulação atual termina após dez quadros, sem acionar uma derrota.

### Alimentos e pontuação

Está confirmado que alimentos geram pontuação adicional. O GDD também descreve combos, multiplicadores e decisões de risco para que o desempenho não dependa apenas da distância.

Tipos de alimentos, valores, regras de captura, combos e multiplicadores ainda precisam ser definidos. Nenhum desses sistemas está implementado. Hoje, a pontuação aumenta uma unidade por atualização e aparece no terminal como distância em metros, sem cálculo físico de distância.

### Moedas e personalização

A coleta de moedas, seu uso na personalização e uma loja de acessórios são decisões fechadas. A intenção é que as corridas contribuam para desbloquear opções de aparência para o personagem.

Coleta, saldo, compras e equipamentos ainda não estão implementados. Preços, catálogo e balanceamento da economia permanecem em aberto.

## Ciclo de uma partida

O GDD descreve o seguinte ciclo para o jogo completo:

**Correr → Desviar → Pular → Coletar → Comer → Pontuar → Sobreviver → Game Over → Evoluir/Comprar → Jogar novamente.**

No protótipo atual, o fluxo executado é:

1. Criar o gerenciador da partida e o personagem.
2. Iniciar a partida no estado `PLAYING`.
3. Simular dez quadros, acionando um pulo no terceiro.
4. Atualizar a física vertical, a pontuação e a velocidade.
5. Exibir os valores no terminal e encerrar a simulação.

## Sistemas descritos no GDD

O documento apresenta uma visão de longo prazo com ranking online semanal, recompensas por colocação, experiência, níveis, desafios, conquistas e desbloqueio de novas regiões. A experiência de corrida é descrita como single-player, com competição por pontuação.

Esses sistemas ainda não estão implementados e precisam de detalhamento. Power-ups, novas regiões e exemplos de acessórios também aparecem no planejamento, sem especificações finais. O escopo obrigatório da versão 1.0 ainda está em aberto.

## Como compilar e executar o protótipo

É necessário um compilador C++ disponível no terminal. O código atual utiliza apenas a biblioteca padrão, sem engine ou biblioteca gráfica integrada.

No Windows, com `g++` instalado e disponível no `PATH`, execute na raiz do projeto:

```powershell
g++ -std=c++17 -Wall -Wextra -Iinclude main.cpp src/GameManager.cpp src/Player.cpp src/Level.cpp -o output/jacarun.exe
.\output\jacarun.exe
```

A pasta `output/` já existe no repositório. O comando gera uma nova compilação do protótipo. A execução mostra dez quadros no terminal, incluindo o salto, o retorno ao chão, a pontuação e a velocidade. Não há janela gráfica nem controles interativos nesta etapa.

## Estrutura do projeto

```text
.
├── README.md
├── main.cpp                 # Ponto de entrada e simulação de dez quadros
├── include/
│   ├── GameManager.h        # Estados e interface de gerenciamento da partida
│   ├── Player.h             # Interface do personagem e atributos do pulo
│   └── Level.h              # Arquivo ainda vazio
├── src/
│   ├── GameManager.cpp      # Início, fim, pontuação e velocidade
│   ├── Player.cpp           # Pulo, gravidade e retorno ao chão
│   └── Level.cpp            # Arquivo ainda vazio
└── output/
    └── main.exe             # Executável já presente no repositório
```

## Definições pendentes

- Controles finais de toque e gesto, movimento de abaixar e regras de colisão.
- Alimentos, valores de pontuação, combos e multiplicadores.
- Obstáculos, geração do percurso e curva de dificuldade.
- Economia, acessórios, preços e funcionamento da loja.
- Regras detalhadas de progressão, ranking e recompensas.
- Engine ou framework, salvamento e integração com Android e iOS.
- Interface, animações, áudio, monetização e escopo da versão 1.0.

## Referência

Este README se baseia no **JACARUN — Game Design Document (GDD), versão 0.1**, com status **Em planejamento**, e no código disponível neste repositório. A seção **22. Decisões fechadas** do GDD registra as definições aprovadas; as demais propostas são apresentadas aqui conforme seu estágio de planejamento e implementação.
