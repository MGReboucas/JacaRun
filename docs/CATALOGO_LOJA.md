# Catálogo da loja — 0.9.32

27 produtos: nove em cada aba (Raios, Moedas e Combos). Cada aba possui três páginas de três cartões. O combo de cada linha entrega os raios e moedas indicados juntos.

| Raios | Preço dos raios | Moedas | Preço das moedas | Preço do combo |
| ---: | ---: | ---: | ---: | ---: |
| 2 | R$ 2,99 | 2.000 | R$ 2,99 | R$ 5,39 |
| 4 | R$ 4,99 | 4.500 | R$ 4,99 | R$ 8,99 |
| 10 | R$ 9,99 | 10.000 | R$ 9,99 | R$ 17,99 |
| 20 | R$ 17,99 | 25.000 | R$ 19,99 | R$ 34,19 |
| 30 | R$ 24,99 | 50.000 | R$ 34,99 | R$ 53,99 |
| 50 | R$ 39,99 | 100.000 | R$ 59,99 | R$ 89,99 |
| 70 | R$ 49,99 | 250.000 | R$ 129,99 | R$ 161,99 |
| 85 | R$ 59,99 | 500.000 | R$ 229,99 | R$ 260,99 |
| 100 | R$ 69,99 | 1.000.000 | R$ 399,99 | R$ 422,99 |

Pacotes maiores reduzem progressivamente o custo por unidade. Combos têm aproximadamente 10% de desconto sobre a soma dos pacotes separados, arredondados para terminar em 9 centavos. Os nove produtos anteriores preservam IDs, valores e quantidades.

Fonte canônica: `server/catalog.json`. O servidor usa esse arquivo para precificação e entrega. O jogo usa `visual/Source/OnlineShopCatalog.h`, gerado por `node scripts/generate-shop-catalog.mjs`; valide com `--check` após alterações. Nunca aceite preço ou quantidade enviados pelo cliente.

IDs novos: `rays_QUANTIDADE`, `coins_QUANTIDADE` e `combo_RAIOS_MOEDAS`. Os três primeiros combos preservam `combo_small`, `combo_medium` e `combo_large`.

Para publicação nas lojas, cadastrar todos os produtos e usar os preços localizados retornados por Google Play/StoreKit. Os preços deste catálogo são propostas de configuração, não produtos já aprovados nas lojas. O APK de teste continua sem cobrança habilitada.
