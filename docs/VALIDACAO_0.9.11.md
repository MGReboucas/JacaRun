# 0.9.11 — salto e continuidade da perseguição

O salto da última cena passa por impulso, subida, ápice, descida e compressão ao aterrissar. Usa as três poses aéreas existentes e emenda no ciclo de oito poses de corrida.

Ao terminar ou pular a história, Jaca entra correndo pela esquerda no menu. O cenário se desloca e Bento chega menor, com cor suavizada e atrás da margem, carregando Dourado. Na partida, Bento permanece brevemente ao fundo e perde distância nos primeiros três segundos.

O pulo jogável também inclina e alonga o corpo na subida, recolhe as pernas no ápice e prepara a descida. Estas alterações são visuais: não mudam colisões, alcance nem tempo do salto físico.

Os controles do menu continuam disponíveis e a corrida só começa quando o jogador pede. As artes existentes foram reutilizadas.

Validação: testes de mecânicas/apresentação, smoke completo com capturas da subida, ápice, aterrissagem e menu conectado; compilação Android e verificação da assinatura. Logs: output/tests-0.9.11-final.log, output/build-0.9.11-visual-final.log e output/build-0.9.11-android-final.log. Teste físico em Android pendente.

24/24 grupos de mecanicas e 10/10 de apresentacao passaram. Smoke Tall e Regular passaram antes do ajuste final de enquadramento; o smoke final Regular valida a aterrissagem mais central e o apice limitado pela altura da tela.

A ultima execucao oculta parou em segundo plano e atingiu o limite de 60 s do script. A repeticao em janela visivel concluiu PASS com o executavel final: output/visual-smoke-0.9.11-final-visible/result.txt. Capturas finais inspecionadas: salto inteiro, aterrissagem dentro da tela e menu correndo com Bento ao fundo.

[Subida](images/jacarun-0.9.11-subida.png) | [Entrada correndo](images/jacarun-0.9.11-corrida.png).
