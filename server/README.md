# Loja JacaRun

**Distribuição confirmada: Google Play e App Store.** O checkout abaixo é exclusivo de Windows/distribuição direta. Android bloqueia checkout externo por padrão, mesmo com uma URL configurada; iOS também bloqueia. Google Play Billing, StoreKit e suas verificações no backend **ainda não estão implementados**. Consulte [publicação nas lojas](../docs/LOJAS_MOBILE.md). A opção CMake `JACARUN_DIRECT_ANDROID_SALES=ON` só deve ser usada para APK fora da Google Play.

Implementação para Windows e APK Android distribuído diretamente, com Checkout Pro do Mercado Pago. **Não está publicada nem habilitada para cobrar.** A configuração do jogo está vazia e a API exige `SHOP_ENABLED=true` para criar pedidos. Testes automatizados usam um provedor falso; não comprovam homologação com o Mercado Pago.

## Executar

Requer Node 24.14 ou superior; não há pacotes externos. SQLite usa WAL, transações e restrições únicas. Execute a partir de `server`:

```powershell
Copy-Item .env.example .env
# Preencha .env no servidor (não no repositório nem no APK).
node --env-file=.env server.mjs
node --test shop.test.mjs http.test.mjs
```

`MP_ACCESS_TOKEN`, `MP_WEBHOOK_SECRET`, `MP_COLLECTOR_ID` e `SHOP_PUBLIC_URL` são obrigatórios. `MP_LIVE=false` exige pagamentos de teste e usa `sandbox_init_point`; `true` exige pagamentos reais. Não misture credenciais/contas de teste e produção. Obtenha o ID do vendedor da conta dona do access token.

Termine TLS em um proxy confiável que encaminhe para `127.0.0.1:8080`. Exponha somente HTTPS, limite corpo a 8 KiB, aplique limites por IP real no proxy e configure timeout. A aplicação não confia em `X-Forwarded-For`: atrás de proxy, seu limite por IP é agregado; mantenha também o limite por jogador e configure a borda para o tráfego previsto. Restrinja permissões do banco, `.env` e backups à conta do serviço. Use disco persistente local e uma instância do serviço; não coloque SQLite em volume de rede. Faça backups consistentes com a API de backup do SQLite, incluindo recuperação testada.

Configure o webhook `https://SEU-DOMINIO/webhooks/mercadopago`, tópico `payment`, com a assinatura secreta da aplicação. O endpoint valida assinatura HMAC, ID, request ID e tolerância de cinco minutos; mantenha relógio sincronizado. Respostas não 2xx devem ser monitoradas e reenviadas pelo provedor. O retorno `/return` apenas orienta o jogador a voltar: nunca concede créditos.

Em `visual/Content/shop-config.json`, coloque somente a origem HTTPS da API em `url`, sem segredo. Recompile Windows/Android. Plataformas sem implementação de transporte nativo falham sem iniciar compra. O transporte não desativa certificados e recusa redirecionamentos da API.

## Fluxo e contrato

1. `POST /v1/sessions` com `{}` cria uma identidade anônima por perfil. O token aleatório tem 256 bits; o banco guarda somente SHA-256. O jogo salva o token antes de permitir checkout. Não é um login recuperável entre aparelhos.
2. `GET /v1/catalog` autenticado retorna 27 SKUs e valores em centavos: nove pacotes de raios, nove de moedas e nove combos. Combos entregam os dois recursos juntos; preços e proporções estão em [publicação nas lojas](../docs/LOJAS_MOBILE.md).
3. `POST /v1/orders`, `Authorization: Bearer TOKEN`, `Idempotency-Key: CHAVE` e `{"sku":"coins_2000"}` cria/reutiliza o pedido. Campos de preço/quantidade enviados pelo cliente são rejeitados. O jogo persiste a chave por pacote para retentar sem gerar novo pedido. Um pedido pago precisa ser verificado; tocar novamente no pacote libera uma nova compra. Pedidos expiram em 24 horas; verifique o saldo antes de recomprar.
4. O jogo abre exclusivamente checkout HTTPS nos hosts autorizados do Mercado Pago no navegador externo. Não recebe dados de cartão.
5. Webhook assinado busca o pagamento diretamente na API do Mercado Pago. Confere ID, referência do pedido, vendedor, ambiente, BRL e valor exato. Somente `approved` gera crédito. Uma transação grava pagamento, pedido e crédito, com unicidade por pedido e por pagamento.
6. `GET /v1/wallet` autenticado retorna totais acumulados de moedas e raios comprados. O botão **VERIFICAR COMPRAS** aplica apenas a diferença dos totais já salvos. Save v8 migra v1–v7; saldo e marcadores de entrega são gravados juntos antes de atualizar a memória. Falha ao salvar permite tentar novamente sem perder o crédito. Não apague os marcadores após consumir itens.

## Homologação antes de ativar

Com credenciais de teste e `SHOP_ENABLED=true`, confirme os 27 pacotes, aprovação, rejeição, pendência, fechamento do navegador, perda de internet, reinício do app e reenvio do webhook. Nos combos, confira a entrega conjunta de raios e moedas. Confirme que nenhum crédito vem do redirect e que dois envios do mesmo evento geram um crédito. Teste dois perfis e confira isolamento. Teste HTTPS com certificado inválido/nome incorreto: a conexão deve falhar. Só depois configure produção, faça uma compra controlada e concilie no painel do vendedor antes de distribuir.

## Limites conhecidos e operação

- O jogo continua offline e o save é local. Um aparelho modificado pode adulterar moedas, token ou marcadores e restaurar backups. A API impede falsificação de pagamento e duplicação no ledger, **não oferece economia autoritativa nem proteção contra adulteração do cliente**. Para esse nível de proteção, consumo, inventário e progresso precisam migrar para uma conta e carteira online. O token local é credencial sensível; quem copia o perfil consegue acessar a identidade. Android mantém backup automático desativado.
- Não há recuperação de conta anônima após apagar dados/reinstalar. Não anuncie restauração entre dispositivos antes de implementar login e suporte a recuperação.
- Reembolso, chargeback ou reembolso parcial bloqueia novas operações da identidade e marca o pedido para revisão. Itens já consumidos offline não podem ser revogados remotamente. O atendimento deve conciliar e decidir a resolução; não existe endpoint público de desbloqueio.
- Um segundo pagamento aprovado para o mesmo pedido falha com 409 e precisa de atendimento/reembolso. Criação de preferência usa idempotência e pode ser repetida após falha; não dependa exclusivamente da idempotência do provedor para entrega.
- Webhook perdido mantém compra pendente até reenvio; ainda não existe reconciliador periódico. Monitore notificações e concilie pagamentos no painel. Eventos recusados por idade exigem reenvio com nova assinatura.
- O estoque local aceita 999 raios. Se o total não couber, nenhum crédito é descartado: use raios e verifique novamente. Falhas da API aparecem como falha de conexão no cliente; o suporte deve consultar logs/painel sem registrar tokens ou dados de cartão.
- Não habilite esta forma de pagamento em distribuição Google Play sem verificar elegibilidade e requisitos de faturamento aplicáveis à conta/região. Google Play Billing é uma integração separada.

Referências: [Checkout Pro Preferences](https://www.mercadopago.com.br/developers/pt/reference/online-payments/checkout-pro-preferences/overview), [assinatura e notificações](https://www.mercadopago.com.br/developers/en/docs/checkout-pro-preferences/additional-settings/optional-notifications), [pagamentos no Google Play](https://support.google.com/googleplay/android-developer/answer/9858738?hl=en).
