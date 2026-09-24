# JacaRun — Cronograma de produção e monetização

**Versão:** 1.0 · **Atualizado em:** 24/09/2026 · **Status:** plano proposto para execução

**Objetivo:** publicar um jogo mobile estável na Google Play e na App Store, com monetização por anúncios e acompanhamento de retenção, custos e receita.

A rota proposta é **Android primeiro, lançamento inicial no Brasil e iOS na sequência**. O planejamento prevê 20 semanas de trabalho distribuídas em 22 semanas de calendário, com duas semanas de baixa disponibilidade no fim do ano. Há uma reserva adicional de quatro semanas para integração, correções e análise das lojas. As datas são estimativas condicionadas às entregas, não uma garantia de aprovação ou receita.

## 1. Ponto de partida

| Área | Situação em 24/09/2026 | O que falta para produção |
| --- | --- | --- |
| Regras do jogo | Protótipo C++ com corrida, pulo, deslize, obstáculos, alimentos, moedas, combos e power-ups | Transformar simulação por comandos em jogo visual com atualização contínua e toque |
| Progressão | Loja cosmética, XP, níveis, recordes e save local | Interface, balanceamento, diretório de dados mobile, migração e recuperação de save |
| Testes | 14 grupos aprovados, incluindo 100 percursos simulados | Testes em aparelhos, integração dos SDKs, anúncios, ciclo de vida e atualização de versão |
| Visual e áudio | Identidade descrita no GDD | Arte final, animações, cenários, UI, música, efeitos e licenças comerciais |
| Android e iOS | Sem projetos mobile ou pacotes das lojas | Engine, toolchains, assinaturas, builds, beta e submissão |
| Anúncios | Não integrados | Contas, SDKs, política etária, consentimento, rewarded, validação e relatórios |
| Serviços online | Não implementados | O primeiro lançamento não depende de login, nuvem ou ranking |
| Ambiente local | Compilação realizada; execução final bloqueada pelo Smart App Control do Windows | Regularizar execução no ambiente de desenvolvimento ou usar ambiente autorizado de testes |

A suíte atual valida regras do protótipo; ela não comprova qualidade mobile nem prontidão para as lojas. O ranking semanal continua na visão do GDD, mas é proposto para uma atualização posterior, evitando colocar backend e antifraude no caminho da primeira receita.

## 2. Premissas, responsáveis e escopo

**Capacidade-base:** um desenvolvedor com 30 horas semanais durante 20 semanas ativas, aproximadamente **600 horas de engenharia**. Arte, áudio, revisão jurídica e testes externos são apoios adicionais, ainda a contratar ou assumir explicitamente. Com apenas 15 horas semanais, planejar aproximadamente o dobro das semanas ativas; datas externas de análise continuam variáveis.

| Papel | Responsabilidade | Disponibilidade necessária |
| --- | --- | --- |
| Responsável pelo produto — proprietário do projeto | Escopo, público, orçamento, contas, contratos e decisão de lançamento | Revisão semanal e decisões em até dois dias úteis |
| Desenvolvimento | Engine, jogo, Android/iOS, SDKs, saves, builds e correções | 30 h/semana |
| Arte e áudio | Personagem, mangue, animações, UI, sons e materiais das lojas | Entregas contratadas entre S4 e S8 |
| QA e testadores | Aparelhos, regressão, feedback e evidências dos testes fechados | Grupo recrutado desde S1; maior atuação em S11–S12 e S20 |
| Privacidade/jurídico e contabilidade | Público menor de idade, dados, publicidade, termos e recebimentos | Consultas em S1 e validação antes do beta monetizado |
| Operação do produto | Suporte, painel de métricas, anúncios e incidentes | Acompanhamento diário após publicar; reservar parte da capacidade do projeto |

**Versão 1.0 proposta:** um mangue bem acabado; um personagem; corrida infinita; pulo e deslize; obstáculos e coletáveis; combos; escudo e ímã; loja cosmética; recordes; progressão local; tutorial; pausa; configurações de áudio; privacidade; suporte; anúncios opcionais para usuários elegíveis.

**Após a primeira versão:** ranking semanal, conta e nuvem, novos biomas, missões e conquistas, eventos, mediação com outras redes e eventual compra para remover publicidade. As moedas da loja inicial são ganhas jogando ou por recompensa de anúncio; não há venda de moeda por dinheiro real no escopo-base.

Esta divisão é uma proposta de priorização. Incluir ranking, múltiplas fases ou compras reais antes da publicação exige reestimar prazo, custo, segurança e regras das lojas.

## 3. Cronograma-base

**Início de referência:** 28/09/2026. Se o início mudar, deslocar as datas mantendo as dependências. S13 e S14 reservam o período de 21/12/2026 a 03/01/2027, sem assumir entregas críticas nas festas.

| Etapa | Semanas e datas | Entregas | Responsável principal | Dependência e critério de conclusão |
| --- | --- | --- | --- | --- |
| 1. Definir lançamento | S1 · 28/09–04/10/2026 | Escopo 1.0, público, política de ads por idade, contas, orçamento, inventário de assets e aparelhos; iniciar recrutamento | Produto + Desenvolvimento | **M1:** decisões registradas; Mac/iPhone e Android disponíveis ou contratados; riscos de privacidade encaminhados |
| 2. Provar a tecnologia | S2–S3 · 05/10–18/10 | Engine estável, núcleo C++ separado da interface, builds Android/iOS de prova, toque, save nativo e anúncio de teste em ambiente adulto controlado | Desenvolvimento | **M2:** demo instalada em Android e iPhone; ponte nativa consegue retornar callback sem travar o jogo |
| 3. Criar a versão visual | S4–S6 · 19/10–08/11 | Corrida em tempo real, câmera, parallax, colisões visuais, animações, áudio inicial e tutorial | Desenvolvimento + Arte | **M3:** sessão de 15 minutos em aparelho sem perda de controles, travamento ou sequência impossível |
| 4. Fechar experiência e conteúdo | S7–S8 · 09/11–22/11 | Arte e som finais do mangue, menus, loja, equipamento visível, configurações, economia, save robusto e instrumentação permitida | Desenvolvimento + Arte | Fluxo completo: abrir → aprender → correr → perder → comprar → fechar → restaurar |
| 5. Integrar monetização e privacidade | S9–S10 · 23/11–06/12 | AdMob, política etária, UMP, rewarded, limites de exibição, eventos de receita, site, suporte e política de privacidade | Desenvolvimento + Produto | **M4:** anúncios de teste e recompensas corretos; recusa, indisponibilidade e modo menor/idade desconhecida não bloqueiam o jogo |
| 6. Beta fechado Android | S11–S12 · 07/12–20/12 | Build assinada, teste fechado, aparelhos diversos, feedback, correções, rascunhos das fichas das lojas | QA + Desenvolvimento | **M5:** evidências de teste; requisito de acesso à produção cumprido quando aplicável; início de coortes D1/D7 |
| Reserva de fim de ano | S13–S14 · 21/12–03/01/2027 | Sem lançamento crítico; manter canais e testes existentes conforme disponibilidade | Produto | Não contar com aprovação das lojas nem plantão normal nesse intervalo |
| 7. Preparar candidata Android | S15 · 04/01–10/01 | Corrigir beta, auditoria de SDKs, performance, privacidade, acessibilidade e formulários; solicitar acesso à produção se necessário | Desenvolvimento + Produto | **M6:** critérios de lançamento cumpridos e acesso à produção concedido antes da etapa seguinte |
| 8. Submeter Android | S16 · 11/01–17/01 | AAB assinado, metadados, screenshots, classificação, declaração de anúncios e instruções de revisão | Produto + Desenvolvimento | Aplicativo enviado; tratar exigências da análise sem prometer prazo de aprovação |
| 9. Lançamento inicial Android | S17 · 18/01–24/01 | Publicação no Brasil, verificação AdMob, liberação de ads após aprovação, monitoramento e correções | Produto + Desenvolvimento | **M7:** instalação pública funcional; receita somente após elegibilidade e aprovação efetiva dos anúncios |
| 10. Finalizar iOS | S18–S19 · 25/01–07/02 | Paridade de conteúdo, adaptação de tela e ciclo de vida, SDK nativo iOS, privacidade e arquivos de assinatura | Desenvolvimento | Build candidata em iPhone; incorporar problemas já descobertos no Android |
| 11. Beta iOS | S20 · 08/02–14/02 | TestFlight, regressão, efeitos dos anúncios, restauro do save e bateria | QA + Desenvolvimento | **M8:** aceite técnico; beta externo depende de análise TestFlight quando exigida |
| 12. Submeter iOS | S21 · 15/02–21/02 | App Store Connect, informações de privacidade, classificação, screenshots e notas de revisão | Produto + Desenvolvimento | Build enviada e pendências respondidas |
| 13. Lançar e estabilizar iOS | S22 · 22/02–28/02 | Publicação após aprovação, ativação elegível de ads, suporte e análise conjunta das plataformas | Produto + Desenvolvimento | **M9:** ambas as lojas operacionais, sem defeitos críticos, com painel de saúde e monetização |
| Reserva adicional | S23–S26 · 01/03–28/03 | Reprovação, atraso de conta, incompatibilidade de SDK, retrabalho e ajustes de produção | Equipe | Usar mediante risco identificado; não preencher antecipadamente com novas funcionalidades |

**Janelas-alvo:** Android em janeiro de 2027; iOS em fevereiro de 2027; reserva até março. As janelas avançam se um marco anterior não for concluído.

**Caminho crítico:** público e contas → prova mobile/ads → jogo visual → privacidade e integração → beta e acesso à produção → submissão → aprovação da loja → verificação/prontidão AdMob → anúncios ativos. Ter o aplicativo publicado não significa que os anúncios já estejam liberados.

## 4. Arquitetura e trabalho técnico

A recomendação inicial é avaliar **Axmol na linha estável/LTS** para manter C++ e compartilhar o jogo entre Android e iOS. Na consulta atual, o projeto apresenta a linha 2.11.x como LTS e a 3.0 como beta; a versão exata deve ser fixada após a prova de compatibilidade, sem assumir uma beta para produção. [Axmol oficial](https://axmol.dev/).

O **Google Mobile Ads C++ SDK foi descontinuado em 2024 e deixou de receber manutenção em 2025**. Para este projeto novo, integrar SDKs nativos suportados, por uma ponte Android e outra iOS. Validar a versão escolhida, suas dependências e eventuais requisitos Families antes de adotá-la. [Descontinuação oficial](https://developers.google.com/admob/cpp/deprecation).

| Componente | Entrega prevista |
| --- | --- |
| Núcleo do jogo | Preservar lógica C++; trocar saída textual por eventos e separar renderização, input e serviços |
| Android | Projeto Gradle/NDK, JNI com Kotlin/Java quando necessário, orientação vertical, controles e ciclo de vida |
| iOS | Projeto Xcode, adaptador Objective-C++/Swift conforme integração, assinatura e ciclo de vida |
| AdService | Interface para elegibilidade, carregamento, exibição, recompensa e erro; implementação falsa para testes |
| Persistência | Diretório privado correto de cada plataforma, gravação atômica, versão de esquema, recuperação e migração |
| Recompensas | ID de corrida/transação e registro persistente para impedir crédito duplicado por callbacks repetidos |
| Observabilidade | Diagnóstico de crashes e eventos mínimos permitidos; revisão de dados coletados por cada SDK |
| Configuração | Controle para desligar ads, limites de frequência e valores, com padrões locais seguros e sem travar o jogo offline |
| Builds | Compilação e testes automatizados; builds Android e iOS reproduzíveis; versões e dependências registradas |
| Segredos e assinatura | Keystores, certificados e credenciais fora do Git; armazenamento protegido e cópia de recuperação sob responsabilidade do titular |

A integração de anúncios deve respeitar a thread exigida pelo SDK e devolver resultados ao núcleo por eventos. Abrir um anúncio pausa a simulação e o áudio quando necessário; fechar, cancelar ou falhar retorna a um estado válido. Não recompensar apenas por fechar a janela.

No Android, validar **arm64, AAB e bibliotecas nativas compatíveis com páginas de 16 KB**, inclusive dependências da engine e SDKs. Essa compatibilidade é especialmente relevante porque o projeto usa C++/NDK. [Guia oficial de páginas de 16 KB](https://developer.android.com/guide/practices/page-sizes).

## 5. Monetização por anúncios

### Plataforma e estratégia inicial

Para anúncios dentro do jogo, utilizar **Google AdMob**. O AdSense é voltado à monetização de sites; pode ser considerado separadamente se futuramente houver um portal com conteúdo. Google Ads é a ferramenta para comprar divulgação, não a origem da receita do aplicativo. [Comparação oficial dos produtos](https://support.google.com/admob/answer/9234653?hl=en-GB).

O objetivo inicial é validar se os jogadores voltam e aceitam uma recompensa opcional. Não colocar anúncios em toda interação para tentar compensar baixa retenção.

| Formato | Proposta para lançamento | Regra inicial de produto, sujeita a teste |
| --- | --- | --- |
| Rewarded — vídeo com recompensa | Formato principal | Após a corrida, oferecer um bônus de moedas informado antes do anúncio, uma vez por corrida |
| Interstitial — tela cheia sem recompensa | Desligado no primeiro piloto; experimentar só após medir retenção | Apenas entre corridas, depois de pelo menos três partidas e 120 segundos desde o último anúncio de tela cheia; nunca junto de um rewarded |
| Banner | Fora da primeira versão | Evitar ocupação da tela vertical e cliques acidentais perto dos controles |
| App open | Fora da primeira versão | Evitar interromper a primeira experiência e o retorno ao jogo |
| Mediação | Depois de obter volume e dados de receita | Comparar receita líquida, falhas, peso dos SDKs e conformidade antes de adicionar redes |

**Bônus proposto:** adicionar até o total de moedas obtidas na corrida, limitado inicialmente a 50 moedas extras e cinco recompensas por dia. Exibir o valor exato; não mostrar oferta quando o bônus for zero. Esses limites são hipóteses de balanceamento, não exigências da AdMob.

A recompensa é moeda virtual de uso exclusivo no JacaRun, sem saque, transferência ou conversão em dinheiro. A escolha de assistir deve ser livre e a recusa não pode retirar ganhos já obtidos. [Política de anúncios com recompensa](https://support.google.com/admob/answer/7313578?hl=en-GB).

### Critérios da integração de ads

- [ ] Criar aplicativos e unidades separados por plataforma e formato.
- [ ] Usar anúncios e dispositivos de teste em desenvolvimento/QA; nunca pedir cliques nos anúncios reais.
- [ ] Checar elegibilidade etária e de privacidade antes de inicializar/carregar o que não for permitido.
- [ ] Dar o bônus apenas no callback de recompensa, com proteção contra duplicação e reinício do aplicativo.
- [ ] Testar cancelamento, callback tardio, perda de rede, ausência de anúncio, retorno do background e toque repetido.
- [ ] Se não houver anúncio, deixar o jogador continuar; não retirar moedas nem aguardar indefinidamente.
- [ ] Registrar impressão e receita quando o SDK permitir, distinguindo solicitação, carregamento e exibição.
- [ ] Permitir desligar publicidade sem impedir as corridas.
- [ ] Separar configuração de teste e produção; conferir IDs antes de enviar à loja.
- [ ] Verificar status da conta, do aplicativo e políticas no painel antes de ativar campanhas de aquisição.

A documentação oficial orienta usar anúncios de teste e tratar a recompensa pelo evento correspondente do SDK. Os nomes exatos dos callbacks dependem da versão nativa selecionada. [Integração rewarded Android](https://developers.google.com/admob/android/next-gen/rewarded).

### Site, verificação e pagamentos

Preparar um site do desenvolvedor com suporte, contato e política de privacidade. Publicar `app-ads.txt` na raiz do domínio com o conteúdo fornecido pela própria conta AdMob e informar esse site nas fichas das lojas. Não preencher IDs inventados. [Configuração de app-ads.txt](https://support.google.com/admob/answer/9363762?hl=en).

Depois que o app estiver vinculado à loja, verificar sua propriedade e acompanhar a análise de prontidão. Novos apps precisam dessa verificação; veiculação plena depende também da aprovação AdMob. Reservar esse intervalo no caixa, pois publicação e monetização não ocorrem necessariamente no mesmo dia. [Verificação do app](https://support.google.com/admob/answer/14538460?hl=en), [prontidão do app](https://support.google.com/admob/answer/10564477?hl=en).

O titular deve concluir as solicitações de identidade, perfil de pagamentos, dados fiscais e bancários apresentadas na conta. Receita estimada no painel não é automaticamente valor já liberado para pagamento. Confirmar condições e limites aplicáveis à conta antes de assumir uma data de recebimento.

## 6. Público, privacidade e menores de idade

O GDD prevê público principal de **10 a 24 anos**, com possível alcance abaixo disso. Portanto, o planejamento trata o público como misto. Não alterar artificialmente a faixa declarada ou a classificação do jogo apenas para liberar anúncios.

**Proposta conservadora para a primeira versão:** jogo acessível conforme a classificação aprovada, com monetização somente para adultos cuja elegibilidade e permissões estejam confirmadas. Para menores ou idade desconhecida, manter a experiência sem publicidade e sem inicialização de rastreamento publicitário. Esta é uma proposta de produto a validar em M1; não garante, sozinha, conformidade legal.

No Brasil, considerar LGPD e ECA Digital, inclusive aferição de idade, proteção por padrão e limitações de publicidade. Não tratar uma simples pergunta de idade como prova suficiente de atendimento a todas as obrigações; avaliar sinais disponíveis da plataforma e obter revisão específica antes do lançamento. O ECA Digital está em vigor desde março de 2026. [Orientações da ANPD](https://www.gov.br/anpd/pt-br/assuntos/eca-digital).

Se futuramente forem exibidos anúncios a crianças ou usuários de idade desconhecida no Android, revisar integralmente Families, versões de SDK certificadas, classificação dos anúncios e ausência de publicidade comportamental. Para público misto, o Google exige tratamento etário apropriado. [Google Play Families](https://support.google.com/googleplay/android-developer/answer/9893335?hl=en-GB).

Na Apple, apps primariamente infantis e a categoria Kids têm restrições adicionais a publicidade e analytics de terceiros; exceções são limitadas. A decisão sobre categoria e audiência deve anteceder a integração final. [Diretrizes App Store, seções 1.3 e 5.1.4](https://developer.apple.com/app-store/review/guidelines/).

| Frente | Entrega e teste |
| --- | --- |
| Inventário de dados | Listar o que jogo, SDKs, diagnósticos e site coletam, finalidade, destinatários, retenção e exclusão |
| Consentimento | Configurar UMP e opções de privacidade aplicáveis; consultar `canRequestAds()` conforme a API; testar aceitar, recusar, erro e alteração da preferência |
| Tracking no iOS | Se houver rastreamento entre empresas ou acesso ao identificador publicitário, aplicar ATT; respeitar recusa e não condicionar recompensa à autorização de tracking |
| Declarações das lojas | Data safety e App Privacy coerentes com o comportamento real do binário e de todos os SDKs |
| Menores/idade desconhecida | Validar que o modo restrito não emite solicitações publicitárias ou telemetria proibida |
| Contato e direitos | Política acessível no app e site; procedimento para suporte e solicitações sobre dados |
| Novos países | Reavaliar consentimento e requisitos locais antes de ampliar a distribuição |

UMP não substitui análise legal, verificação etária nem ATT. A documentação do Google define o fluxo técnico de consentimento; a Apple estabelece quando ATT é necessário. [UMP Android](https://developers.google.com/admob/android/privacy), [privacidade e uso de dados Apple](https://developer.apple.com/app-store/user-privacy-and-data-use/).

## 7. Requisitos das lojas e processo de publicação

As exigências abaixo foram consultadas em **24/09/2026**. Revalidar em M2 e imediatamente antes de cada submissão, pois o plano atravessa 2027.

| Plataforma | Preparação obrigatória | Evidência antes de enviar |
| --- | --- | --- |
| Google Play | Conta verificada; identificador definitivo; assinatura e chave de upload protegida; AAB; classificação; audiência; declaração de anúncios; Data safety; privacidade; ficha e imagens | Instalação pelo canal de testes, atualização preservando save e relatórios de testes |
| App Store | Apple Developer; Mac/Xcode compatível; identificador, certificados e perfis; App Store Connect; TestFlight; classificação; App Privacy; manifests e declarações de APIs exigidas; screenshots, suporte e privacidade | Instalação via TestFlight, logs de testes e instruções de revisão |

**Android:** a exigência consultada para novos apps e atualizações comuns é **target Android 16 / API 36 ou superior desde 31/08/2026**. O `minSdk` é uma decisão diferente, a escolher após definir os aparelhos suportados. [Requisitos de target API](https://support.google.com/googleplay/android-developer/answer/11926878?hl=en).

**Conta pessoal nova no Google Play:** contas pessoais criadas após 13/11/2023 precisam, quando abrangidas pela regra, de pelo menos **12 testadores inscritos continuamente por 14 dias** no teste fechado antes de solicitar acesso à produção. Recrutar 20 pessoas como margem operacional e verificar os dados no Console; cumprir a quantidade não substitui a análise de acesso. [Regras oficiais de teste](https://support.google.com/googleplay/android-developer/answer/14151465?hl=en).

**Apple:** desde 28/04/2026, uploads exigem Xcode 26 ou superior e SDK iOS 26 ou superior, conforme a exigência consultada. Isso não significa obrigatoriamente exigir iOS 26 como versão mínima para jogar. O primeiro build para teste externo pode depender de análise TestFlight. [Requisitos Apple](https://developer.apple.com/news/upcoming-requirements/), [TestFlight](https://developer.apple.com/help/app-store-connect/test-a-beta-version/testflight-overview/).

**Sequência operacional de lançamento:**

1. Congelar funcionalidades da candidata e atribuir versão/build.
2. Compilar em ambiente controlado, executar testes e arquivar artefatos e símbolos.
3. Validar a instalação pela loja de testes, não apenas por cabo.
4. Conferir audiência, privacidade, ads, classificação, URLs, imagens e licenças.
5. Submeter, responder à análise e corrigir os problemas apontados.
6. Publicar inicialmente no Brasil, após aprovação, e limitar a divulgação até medir estabilidade.
7. Vincular a versão publicada ao AdMob, concluir verificação/prontidão e ativar anúncios apenas quando permitido.
8. Monitorar, corrigir e ampliar divulgação/distribuição de forma planejada.

Não presumir que a primeira publicação permite rollout por percentual de usuários. Usar seleção de países e aquisição controlada no lançamento; avaliar recursos de rollout gradual para atualizações conforme a loja. [Publicação no Google Play](https://support.google.com/googleplay/android-developer/answer/9859348?hl=en).

## 8. Critérios para autorizar lançamento

Os números abaixo são **metas internas propostas**, não requisitos oficiais das lojas nem resultados já alcançados.

| Área | Critério de aceite |
| --- | --- |
| Correção | Nenhum defeito conhecido de severidade crítica/alta: crash reproduzível, perda de save, bloqueio de partida, coleta indevida de dados ou crédito duplicado |
| Mecânicas | Suíte de regressão aprovada; sequências atravessáveis; colisão visual e lógica consistentes |
| Aparelhos | Testes em Android de entrada/intermediário e iPhone do limite de suporte; diferentes proporções de tela e versão de sistema |
| Performance | Alvo de 60 FPS no aparelho de referência e ao menos 30 FPS estáveis no aparelho mínimo; medir temperatura e memória em sessão de 20 minutos |
| Ciclo de vida | Chamadas/notificações, troca de app, tela bloqueada, saída forçada, retorno de anúncio e retomada sem avanço indevido da corrida |
| Persistência | Instalação limpa, atualização, arquivo inválido, pouco armazenamento e migração de esquema sem perda de progresso válido |
| Anúncios | Consentimento/idade corretos; anúncios de teste aprovados; offline/no-fill sem bloqueio; recompensa exatamente uma vez |
| Qualidade do beta | Meta de pelo menos 99,5% de sessões sem crash, com amostra registrada; amostra pequena não comprova estabilidade |
| Loja e operação | Documentos consistentes, política publicada, suporte funcional, responsável de plantão e processo de correção definido |

**Retenção antes de escalar anúncios pagos:** medir D1 e D7 em coortes comparáveis; hipótese inicial de avaliação de D1 ≥ 25% e D7 ≥ 8%. Usar esses valores para orientar investigação, não como benchmark garantido. Buscar 100–300 instalações de piloto quando possível, informar o tamanho da amostra e não tirar conclusões fortes de um grupo mínimo de 12 testadores.

## 9. Métricas, receita e aquisição de jogadores

| Pergunta | Indicador | Uso |
| --- | --- | --- |
| As pessoas entendem o jogo? | Conclusão do tutorial, tempo até primeira corrida e desistência inicial | Melhorar onboarding antes de aumentar anúncios |
| Elas voltam? | Retenção D1/D7 por coorte e sessões por usuário | Priorizar diversão, dificuldade e progressão |
| Os anúncios estão disponíveis? | Taxa de preenchimento, falhas de carga, exibição e finalização | Encontrar problemas de integração e demanda |
| Os anúncios compensam? | Receita estimada por formato, país e plataforma; eCPM; receita diária por usuário elegível | Comparar formatos sem olhar apenas cliques |
| A publicidade prejudica o jogo? | Retenção, duração de sessão e reclamações por frequência de ads | Reduzir pressão publicitária quando piorar a experiência |
| Vale comprar divulgação? | Custo por instalação e valor acumulado por usuário/coorte | Investir apenas com hipótese de retorno e limite de perda definido |

Instrumentar eventos como início/fim de corrida, motivo de derrota, compra cosmética, oferta de recompensa, anúncio carregado/exibido, recompensa e erro. Separar ambientes de teste/produção e limitar dados por idade, consentimento e finalidade. Não incluir nome, e-mail ou data de nascimento em eventos de gameplay.

### Modelo simples de receita

**Receita mensal estimada = DAU monetizável × sessões por dia × impressões efetivamente exibidas por sessão × 30 ÷ 1.000 × eCPM.**

DAU significa usuários ativos por dia. DAU monetizável considera apenas quem é elegível para receber anúncios; não inclui automaticamente todo jogador. Impressões efetivas já refletem disponibilidade e exibição, portanto não multiplicar preenchimento uma segunda vez.

Exemplo puramente matemático: **1.000 usuários elegíveis/dia × 2 sessões/dia × 1 impressão real/sessão = 60.000 impressões/mês.**

| eCPM hipotético | Receita mensal estimada ao editor, antes de seus custos e tributos |
| --- | ---: |
| US$ 2 | US$ 120 |
| US$ 5 | US$ 300 |
| US$ 10 | US$ 600 |

Esses eCPMs **não são uma previsão para o JacaRun nem uma média de mercado**. A receita real depende de audiência, país, formato, consentimento, demanda e ajustes por tráfego inválido. Utilizar dados reais do piloto antes de projetar lucro.

**Ponto de equilíbrio:** custos mensais na mesma moeda ÷ receita média mensal por usuário elegível. Para aquisição paga, comparar custo de aquisição com o valor líquido acumulado de coortes maduras; receita de um único dia não equivale a valor vitalício.

Primeiro preparar página da loja, vídeo curto de gameplay e materiais de divulgação; validar aquisição orgânica e feedback. Só depois testar mídia paga com orçamento pequeno e teto definido pelo proprietário. Não comprar instalações artificiais, incentivar cliques ou gerar tráfego próprio para anúncios. [Política de tráfego inválido](https://support.google.com/adsense/answer/2753860?hl=en).

## 10. Orçamento e recursos

| Item | Referência ou decisão necessária | Quando |
| --- | --- | --- |
| Google Play Console | Taxa de cadastro de US$ 25, pagamento único, conforme página consultada; confirmar cobrança local | S1 |
| Apple Developer Program | US$ 99/ano ou valor local aplicável; confirmar durante adesão | S1–S3 |
| Engenharia | 600 horas-base + até 120 horas de reserva; custo = horas × custo-hora escolhido | Aprovar em S1 |
| Arte, animação, UI e áudio | Cotar entregas com licença comercial e arquivos editáveis | Contratar antes de S4 |
| Mac e iPhone | Compra, equipamento já disponível ou serviço/locação autorizado; necessário para validar iOS | Resolver até S2 |
| Androids de teste | Pelo menos perfis de entrada e intermediário; aparelhos próprios ou acesso contratado | Resolver até S3 |
| Site/domínio/suporte | Cotar domínio, hospedagem estática, e-mail e manutenção | S1–S9 |
| Jurídico/contabilidade | Cotar análise de público, dados, ads e recebimentos | S1 e antes de M4 |
| Serviços e diagnóstico | Definir limites de uso e alertas de custo antes de produção | S8–S10 |
| Aquisição de usuários | Orçamento separado, liberado somente após piloto e revisão das métricas | Após M7 |

As taxas oficiais não incluem hardware, pessoal, câmbio, tributos ou serviços opcionais. [Cadastro Play Console](https://support.google.com/googleplay/android-developer/answer/6112435?hl=en), [adesão Apple Developer](https://developer.apple.com/help/account/membership/program-enrollment).

Preencher em S1: orçamento total disponível, limite mensal de operação, limite de aquisição e meses de caixa sem receita. **Não financiar o desenvolvimento supondo que a receita de ads começa no dia da publicação.**

## 11. Operação e riscos

| Risco | Resposta planejada | Responsável |
| --- | --- | --- |
| Engine ou SDK incompatível com os builds atuais | Prova em aparelhos reais em S2–S3; fixar versões suportadas antes de produzir conteúdo | Desenvolvimento |
| Restrição de ads por idade ou categoria | Decisão etária e revisão desde S1; manter jogo funcional sem publicidade | Produto + Privacidade |
| Atraso de conta, testadores ou análise da loja | Iniciar cadastro/recrutamento cedo; acompanhar 14 dias quando aplicável; usar reserva | Produto |
| Falha de integridade do save/recompensa | Transação identificada, testes de interrupção e migração; rollback de configuração de ads | Desenvolvimento |
| Retenção baixa | Melhorar tutorial, ritmo, feedback e economia antes de ampliar anúncios ou mídia paga | Produto |
| Receita menor que o custo | Controlar orçamento, medir coortes e interromper aquisição que não se sustenta | Produto |
| Reprovação AdMob ou tráfego suspeito | Verificar painel de políticas, suspender divulgação inadequada e corrigir a causa | Operação |
| Crash na versão pública | Desligar ads se relacionados, conter divulgação/rollout e publicar correção com versão maior | Desenvolvimento |
| Ausência de suporte após publicar | Definir contato, plantão e capacidade semanal de manutenção | Produto |

No primeiro mês, revisar saúde e anúncios diariamente e retenção semanalmente. Manter os símbolos das builds, histórico de versões, inventário dos SDKs e cópia das chaves. Fazer atualização preventiva de dependências e rever exigências das lojas antes de cada release.

Uma versão instalada não pode ser simplesmente desfeita em todos os aparelhos: a resposta usual é conter novas instalações/atualizações quando possível, desligar uma configuração problemática e publicar uma correção. O save precisa permanecer compatível.

## 12. Primeiras ações e acompanhamento

- [ ] Confirmar dedicação semanal, orçamento e responsável por cada papel.
- [ ] Aprovar escopo 1.0 e adiar formalmente ranking/backend para outra versão.
- [ ] Decidir o tratamento etário e validar a proposta de ads apenas para adultos elegíveis.
- [ ] Criar/verificar Play Console, Apple Developer e AdMob sob titularidade correta.
- [ ] Garantir acesso a Android, Mac e iPhone.
- [ ] Escolher engine estável após prova com renderização, toque e anúncio de teste.
- [ ] Definir nome final, identificadores dos apps e direitos de marca/assets.
- [ ] Recrutar testadores e registrar disponibilidade, sem publicar seus dados no repositório.
- [ ] Criar backlog com tarefa, responsável, estimativa, dependência e critério de aceite.
- [ ] Revisar este cronograma toda semana com base em demonstração executável e impedimentos reais.

| Registro semanal | Preenchimento |
| --- | --- |
| Semana/data | A preencher |
| Marco em andamento | M1 a M9 |
| Entregas demonstradas | Links para build, commit e evidência de teste |
| Horas previstas/realizadas | A preencher |
| Impedimentos e responsáveis | A preencher |
| Custo acumulado e saldo de orçamento | A preencher |
| Métricas disponíveis | Amostra, período e plataforma |
| Próxima decisão | Responsável e prazo |

**Próxima entrega concreta:** M1 e M2 — fechar público/escopo e colocar uma prova visual em Android e iPhone, preservando o núcleo C++ e validando a integração nativa de anúncios de teste.
