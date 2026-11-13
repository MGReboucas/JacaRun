# Google Play e App Store

Destino confirmado: distribuir JacaRun pela Google Play e pela App Store. O APK de desenvolvimento não é o artefato final de publicação e não pode ser convertido em aplicativo iOS. O projeto nativo iOS existente em `visual/proj.ios_mac/ios` precisa ser compilado, assinado e testado com Xcode.

As contas Google Play e Apple Developer já existem, conforme informado pelo responsável. Não há Mac local. A opção recomendada para este projeto C++/Axmol é um build macOS na nuvem, por exemplo Codemagic com workflow iOS nativo. EAS é voltado a Expo/React Native e não fornece uma conversão direta deste projeto. Ainda não há workflow de nuvem configurado ou executado.

O processo remoto deve obter a mesma revisão Axmol usada aqui (`v2.11.4`, commit `b14941e6f50a0ce12489bd8f57041e093fb58819`), preparar dependências para macOS, gerar o projeto Xcode com CMake e compilar para iPhone. A assinatura usa certificados/perfis da conta Apple configurados como segredos no serviço. Depois de gerar o IPA, validar no TestFlight em iPhone real antes de enviar à App Store. O script atual `setup-visual.ps1` prepara ferramentas Windows e não deve ser executado sem adaptação no runner macOS.

## Estado real da implementação

- A loja tem três abas (Raios, Moedas e Combos), 27 pacotes em nove páginas, botões e armazenamento idempotente dos totais comprados.
- O servidor existente verifica apenas pagamentos Mercado Pago. Ele **não** valida recibos Apple ou tokens Google.
- O jogo Android, por padrão, impede abertura do Mercado Pago mesmo se alguém configurar a URL da API. iOS também impede esse caminho. Isso não implementa compras nativas: os botões informam indisponibilidade enquanto a integração específica não existir.
- O modo direto opcional `-DJACARUN_DIRECT_ANDROID_SALES=ON` é exclusivo de APK distribuído fora da Google Play. Não habilitar nos builds enviados à loja.
- Não existe ainda AAB de produção assinado, build iOS homologado, conta recuperável nem inventário autoritativo no servidor.

## Produtos a cadastrar nas duas lojas

O catálogo completo agora contém 27 produtos, até 100 raios e 1.000.000 de moedas. Consulte [todos os pacotes e preços](CATALOGO_LOJA.md) e `server/catalog.json`. A tabela abaixo registra os nove produtos originais; os demais seguem os IDs definidos no catálogo.

Todos são consumíveis. IDs propostos devem ser confirmados antes de cadastrar ou publicar; use IDs existentes se os produtos já tiverem sido criados.

| SKU interno | Produto sugerido | Quantidade | Preço BRL proposto |
| --- | --- | ---: | ---: |
| rays_2 | jacarun_rays_2 | 2 raios | 2,99 |
| rays_4 | jacarun_rays_4 | 4 raios | 4,99 |
| rays_10 | jacarun_rays_10 | 10 raios | 9,99 |
| coins_2000 | jacarun_coins_2000 | 2.000 moedas | 2,99 |
| coins_4500 | jacarun_coins_4500 | 4.500 moedas | 4,99 |
| coins_10000 | jacarun_coins_10000 | 10.000 moedas | 9,99 |
| combo_small | jacarun_combo_small | 2 raios + 2.000 moedas | 5,39 |
| combo_medium | jacarun_combo_medium | 4 raios + 4.500 moedas | 8,99 |
| combo_large | jacarun_combo_large | 10 raios + 10.000 moedas | 17,99 |

Combos juntam os pacotes individuais equivalentes com aproximadamente 10% de desconto, arredondados para preços terminados em 9 centavos. Economias: R$ 0,59, R$ 0,99 e R$ 1,99. A concessão de um combo inclui as duas moedas de jogo na mesma transação, sem duplicação ao reenviar a confirmação.

Os preços exibidos no aplicativo devem vir da consulta de produtos da loja, com moeda e formatação locais. A tabela atual do jogo não deve ser usada como preço definitivo para vendas nas lojas.

## Integrações necessárias

**Google Play:** conectar BillingClient, consultar produtos, abrir o fluxo de compra, tratar cancelamento e compra pendente, recuperar compras ao retomar o aplicativo e enviar o token ao backend autenticado. O servidor deve validar pela Google Play Developer API, conferir package name, produto, estado e vínculo ao jogador, registrar a entrega com unicidade do token e consumir a compra somente após registrar a concessão. Retentativas após falha precisam recuperar a concessão existente e concluir o consumo. Configurar notificações de alterações/reembolsos e conciliação. Publicar AAB assinado em teste interno antes de produção.

**App Store:** integrar StoreKit, carregar produtos e preços, tratar transações verificadas, pendentes e inacabadas, enviar transação ao backend e finalizar somente depois da entrega persistida. O servidor deve validar transação assinada/consultar App Store Server API, conferir bundle ID, produto, ambiente e vínculo ao jogador, e rejeitar reutilização de transação. Configurar App Store Server Notifications e conciliação. Compilar no Mac, testar StoreKit Sandbox e TestFlight e depois enviar à revisão.

**Compartilhado:** migrar a identidade anônima para conta recuperável se houver restauração entre dispositivos. Para proteção da economia contra adulteração local, migrar inventário e consumo para o servidor. Não reconhecer/consumir/finalizar uma compra rejeitada pelo backend ou ainda sem entrega durável. Não usar o redirect do navegador como confirmação.

## Dados e acessos pendentes

- Confirmar contratos comerciais e dados de recebimento nas contas Google Play Console e Apple Developer já existentes.
- Package name Android e bundle ID iOS definitivos; IDs dos produtos já existentes, se houver.
- Domínio HTTPS do backend.
- Credenciais Google/Apple provisionadas no servidor, nunca no APK, no app iOS ou no chat.
- Chave de assinatura Android e configuração de CI macOS com assinatura Apple (sem Mac local).

Não iniciar cobranças reais até validar compra, pendência, falha de rede, reabertura, repetição da transação, troca de conta e reembolso nos ambientes de teste das duas lojas.

Referências oficiais: [Google Play Billing](https://developer.android.com/google/play/billing/integrate), [ciclo de compras consumíveis](https://developer.android.com/google/play/billing/lifecycle/one-time), [compras no App Store Connect](https://developer.apple.com/help/app-store-connect/configure-in-app-purchase-settings/overview-for-configuring-in-app-purchases/), [diretrizes da App Store](https://developer.apple.com/app-store/review/guidelines/).

Build remoto: [Codemagic para iOS nativo](https://docs.codemagic.io/yaml-quick-start/building-a-native-ios-app/), [escopo do EAS Build](https://docs.expo.dev/build/introduction/).
