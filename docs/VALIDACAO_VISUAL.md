# Registro de validação — protótipo visual 0.3

**Data:** 24/09/2026. **Escopo:** revisão do protótipo após o vídeo Android; não é aceite de produção.

## Feedback e correções

O vídeo `example-android.mp4` enviado pelo usuário tem aproximadamente 33,5 segundos. Seus quadros mostram a versão anterior funcionando em Android, chegando a cerca de 336 metros, com faixas vazias, botões grandes e objetos removidos perto do jacaré. Modelo do aparelho e versão Android não foram informados.

A revisão 0.3 preenche a altura real da tela com cenário, remove os botões da corrida, amplia/detalha o personagem e os coletáveis e compacta o HUD. Gestos verticais respondem durante o movimento; toque rápido com dois dedos pausa. Menu, pausa e resultado continuam com suas ações e também aceitam gesto para cima.

A causa do desaparecimento era desenhar apenas a lista de eventos futuros de colisão. Agora cada entidade possui um ID, o núcleo informa o resultado da interação e a apresentação mantém obstáculos/itens perdidos até a borda esquerda. Somente itens realmente coletados terminam uma animação curta antes de serem removidos. O objeto continua visível sem gerar nova colisão ou recompensa.

![Objetos ainda visíveis após a passagem](images/objetos-persistentes.png)

## Resultados

| Verificação | Resultado | Evidência |
| --- | --- | --- |
| Núcleo C++/MSVC | Aprovado | 14/14 grupos, incluindo 100 percursos procedurais |
| Apresentação e gestos sem engine | Aprovado | 3/3 grupos: permanência/coleta, colisão fatal/eventos, gestos |
| Janela 432 × 810 | Aprovado | Prova visual automatizada com o binário final |
| Janela 360 × 788 | Aprovado | Proporção dos quadros do vídeo; cenário até as bordas, HUD e overlays conferidos |
| Gestos no dispatcher Axmol | Aprovado | Pulo, deslize, dois dedos para pausar, retomada/reinício por gesto |
| Obstáculo ultrapassado e moeda perdida | Aprovado | Permanecem atrás do jogador; captura dedicada e testes de descarte fora da tela |
| Coleta e colisão | Aprovado | Recompensa uma vez, animação de coleta e retenção do obstáculo fatal |
| Segundo plano simulado, save e reinício | Aprovado | Pausa sem avanço; leitura de perfil isolado; saldo preservado |
| APK Android arm64 0.3.0 | Compilado | `build-android.ps1 -SkipSetup` |
| Manifesto | Inspecionado | `com.mgreboucas.jacarun`, versionCode 2, min API 23, target 36 |
| Assinatura Debug | Aprovada | `apksigner verify --verbose`: v1/v2 válidas |
| ZIP/ELF de 16 KB | Aprovado estaticamente | `zipalign -c -P 16 -v 4`; LOAD de ambas as bibliotecas em `0x4000` |
| Android físico 0.2 | Evidência recebida | Vídeo do usuário; sessão curta, sem verificação de segundo plano/save |
| Android físico 0.3 | Pendente | Instalar atualização e retestar no aparelho |
| Execução em páginas de 16 KB | Pendente | Precisa de aparelho/emulador apropriado |
| iOS e AdMob | Pendentes | Nenhuma build Apple ou integração de publicidade nesta revisão |

## Ambiente e artefatos

- Axmol 2.11.4, commit `b14941e6f50a0ce12489bd8f57041e093fb58819`; axslcc 1.14.0.
- Visual Studio 2026/MSVC 19.51; CMake 4.2.3-msvc3.
- Android NDK r27c; JBR 21.0.10; SDK 36/build-tools 36.0.0; Ninja 1.13.2.
- Gradle 9.2.1; Android Gradle Plugin 8.11.1.
- Windows: `visual/build-win32/bin/JacaRun/Debug/JacaRun.exe`.
- Android: `output/JacaRun-0.3.0-debug.apk`; atalho para o último build em `output/JacaRun-debug.apk`.
- APK: **23.994.508 bytes**.
- SHA256: `dcb25fef94abcea200560283322b1f0c63f1ff2b0ce12e51b9ebf722b940c6fa`.
- Testes CTest: `build/Testing/Temporary/LastTest.log`.
- Prova visual final regular: `output/visual-smoke-20260924-120818/`.
- Prova visual final comprida: `output/visual-smoke-20260924-120633/`.
- Build Android: `output/android-03-final.log`.
- Verificações: `output/android-03-package.log`, `output/android-03-signature.log`, `output/android-03-alignment.log` e `output/android-03-elf-alignment.log`.

O helper recria somente o APK gerado antes de empacotar, evitando espaço vazio acumulado pelo ZIP incremental. O hash identifica esta compilação; recompilações podem produzir bytes diferentes. O vídeo original, seus quadros de análise, logs, APKs, executáveis, caches e saves não são enviados ao Git.

## Limites e próximo reteste

Os testes de gestos usam eventos sintetizados, não uma touchscreen física. A revisão mantém colisões pontuais do núcleo; ajustar a sensação visual após o reteste. Arte e áudio finais, performance/bateria e sessão de 15 minutos continuam pendentes. Avisos de toolchain e de entradas auxiliares META-INF permanecem nos logs; não foram ocultados.

No Android usado para o vídeo:

1. Instalar `JacaRun-0.3.0-debug.apk` como atualização, sem desinstalar a versão anterior.
2. Conferir moedas, recorde e acessórios preservados; registrar modelo e Android.
3. Deslizar para cima/baixo em regiões diferentes da tela; confirmar resposta durante o movimento e ausência de botões na corrida.
4. Observar raízes/galhos e itens perdidos atravessando a tela depois do jacaré; itens obtidos devem animar e contabilizar uma vez.
5. Pausar com dois dedos, retomar por gesto, ir à tela inicial do Android e voltar: a corrida deve permanecer pausada.
6. Jogar por 15 minutos e verificar notch, navegação por gestos, estabilidade e save após reabrir.

[Voltar ao cronograma](../CRONOGRAMA_PRODUCAO.md).
