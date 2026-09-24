# Registro de validação — protótipo visual 0.2

**Data:** 24/09/2026. **Escopo:** prova técnica local; não é aceite de produção.

| Verificação | Resultado | Evidência |
| --- | --- | --- |
| Núcleo C++ compilado com MSVC | Aprovado | CMake/CTest; 14/14 grupos, incluindo 100 percursos |
| Aplicativo Axmol Windows x64 Debug | Compilado e executado | `build-visual.ps1 -Smoke` |
| Interface com toque/gesto sintetizado no dispatcher da engine | Aprovado | Botão pulo, gesto deslize, retomada e reinício |
| Integração das regras na cena | Aprovado | Duas passagens por obstáculos, moeda, alimento e colisão final |
| Pausa de ciclo de vida | Aprovado no cenário automatizado Windows | Distância inalterada até retomar |
| Save/releitura e reinício | Aprovado | Perfil isolado; saldo preservado e distância zerada |
| Revisão visual | Menu, corrida e resultado conferidos | Capturas em `docs/images` e diretório local do teste |
| APK Android arm64 Debug | Compilado | `assembleDebug`; helper reproduzido com sucesso |
| Manifesto APK | Inspecionado | ID `com.mgreboucas.jacarun`, versão 0.2.0/code 1, min API 23, target 36 |
| Assinatura Android de desenvolvimento | Verificada | `apksigner verify` retornou 0 |
| Alinhamento do APK | Aprovado | `zipalign -c -P 16 -v 4` |
| Segmentos ELF das bibliotecas nativas | Aprovado estaticamente | `libJacaRun.so` e `libc++_shared.so`: todos os LOAD com `0x4000` |
| Aparelho Android físico | Não executado | `adb devices` sem dispositivos conectados |
| Execução em páginas de 16 KB | Não executada | Precisa de aparelho/emulador apropriado |
| iOS | Não compilado/testado | Windows sem Mac/Xcode/iPhone nesta entrega |
| AdMob/consentimento/recompensas de anúncios | Não implementado | Etapas futuras do cronograma |

## Ambiente

- Axmol 2.11.4, commit `b14941e6f50a0ce12489bd8f57041e093fb58819`.
- axslcc 1.14.0.
- Visual Studio 2026/MSVC 19.51; CMake 4.2.3-msvc3.
- Android NDK r27c, JBR 21.0.10, SDK 36/build-tools 36.0.0, Ninja 1.13.2.
- Gradle 9.2.1 e Android Gradle Plugin 8.11.1.

## Artefatos locais

- Windows: `visual/build-win32/bin/JacaRun/Debug/JacaRun.exe`.
- Android: `output/JacaRun-debug.apk` e saída original em `visual/proj.android/app/build/outputs/apk/debug/`.
- APK: 23.928.983 bytes.
- SHA256: `6456accda034048233e7b15cb46024741e5e8c62af79075b8b4450a85cc6ce35`.
- Testes do núcleo: `build/Testing/Temporary/LastTest.log`.
- Teste visual: `output/visual-smoke-20260924-113351/`.
- Build Android: `output/android-build.log`; reprodução do helper: `output/android-script-verify.log`.
- Inspeção Android: `output/android-package.log`, `output/android-alignment.log` e `output/android-elf-alignment.log`.

O hash identifica esta compilação; outra compilação pode gerar bytes diferentes. Logs, APKs, executáveis, caches, arquivos locais e saves permanecem fora do Git.

## Limitações observadas

O toolchain produziu avisos internos da engine/NDK, depreciações Gradle/Java e um aviso de runtime no primeiro link Windows Debug. O verificador de assinatura também apontou entradas auxiliares de META-INF não cobertas pela assinatura JAR; a verificação global foi aprovada. Esses avisos não foram ocultados nem tratados como validação para uma release de loja.

O teste visual usa eventos sintetizados no processo, não dedos em touchscreen nem transições reais do sistema Android/iOS. Ainda falta conferir multi-toque, back/home, recortes e proporções de tela, consumo de bateria, áudio, performance em aparelhos e restauração após o sistema matar o processo. A sequência de 15 minutos em aparelho exigida por M3 permanece aberta.

## Próximo aceite: MOB-01

1. Instalar o APK em Android físico autorizado.
2. Registrar modelo, versão Android e tamanho/proporção da tela.
3. Testar botões e gestos, pulo/deslize junto aos obstáculos e legibilidade.
4. Ir para segundo plano e retornar: a corrida deve estar pausada, sem avanço oculto.
5. Encerrar a corrida, comprar/equipar um acessório, fechar e reabrir: verificar saldo, recorde e equipamento.
6. Jogar por 15 minutos, registrar falhas e atualizar o cronograma com evidências.

[Voltar ao cronograma](../CRONOGRAMA_PRODUCAO.md).
