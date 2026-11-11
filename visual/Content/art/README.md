# Direção visual — JacaRun

Artes produzidas em 10/11/2026 com o gerador integrado imagegen, a partir da referência de personagem e ambientes fornecida pelo usuário. Os PNGs ficam neste diretório e são empacotados pelo fluxo normal de Content do Axmol.

- `jaca-atlas.png`: corrida em quatro poses, salto, deslize, mordida e derrota; transparência preservada. Os recortes explícitos estão em `MainScene::drawCrocodile`.
- `objects-atlas.png`: raiz, tronco, pedra, galho, caranguejo, peixe, peixe raro e moeda.
- `ground-atlas.png`: margem de areia, píer e solo de floresta.
- `mangue.png`, `vila.png`, `floresta.png`: fundos ilustrados de 1536 × 1024.
- `logo.png`: marca ilustrada do menu.

Desde a 0.9.1, os ambientes alternam automaticamente a cada 500 m: mangue, vila, floresta e novamente mangue. A mistura entre os fundos e pisos ocupa os primeiros 40 m de cada novo trecho. A pausa congela a transição e uma nova partida reinicia o ciclo. Não há seleção no menu nem leitura da preferência antiga. Eles compartilham as regras de corrida existentes; novos alimentos, regras e desbloqueios por bioma não fazem parte desta alteração. O deslocamento do piso acompanha a simulação e o fundo se move mais lentamente. Tiles alternados espelhados evitam uma quebra brusca nas bordas dos fundos.

O personagem usa frames ligados ao estado da simulação. Os acessórios existentes continuam como sobreposições, tanto na loja quanto na corrida. As caixas de colisão e os preços não foram alterados.

## Prompts utilizados (gerador integrado)

### Marca

Create only the JacaRun game logo from attached reference, as a polished transparent PNG asset, landscape 1536x1024. Exact text: JacaRun. Big playful chunky rounded letters, Jaca in warm yellow-orange gradient, Run in vivid lime-green gradient, cream inner keyline and thick dark brown outer outline, dimensional wooden backing silhouette, small tropical palm leaves above. Match reference logo closely. Logo fills central 90% width and 65% height. No crocodile, no background, no additional text, no panels, no scenery. Actual transparent alpha outside the logo. Center whole logo without cropping.

### Personagem

Create a production game sprite atlas based on the attached JacaRun character reference. Transparent background, no text, no grids. Canvas 1536x1024 with exactly 4 columns and 2 rows of equal cells (384x512). Each cell contains the SAME cheerful green baby crocodile with yellow belly, backward red baseball cap, red neck bandana, small brown backpack, thick dark outline, rich hand painted cartoon shading matching reference. Full body facing RIGHT strict side view. Keep size and baseline consistent: feet at 88% cell height, entire character within inner 85% of cell, including tail. Top row: four distinct consecutive RUN cycle poses (contact, down, passing, up), visibly alternating legs and arms. Bottom row: jumping with feet tucked; low sliding pose; mouth wide open eating pose; dizzy collapsed game-over pose. No other characters, props, floor, shadows, particles, lettering, borders or background. Crisp polished game-ready sprite sheet.

Refinamento: Remove the ENTIRE dark colored background from this sprite atlas and replace it with actual transparent alpha (zero alpha). Preserve all eight crocodile poses exactly, their colors, positions, sizes, layout. No shadow, no glow, no background pixels between characters. This must be a transparent PNG sprite sheet, not an illustration on a dark backdrop. Preserve original 1536x1024 dimensions and 4-column 2-row alignment.

### Mangue

Use case: stylized-concept. Production background for a side-scrolling JacaRun game, matching attached reference's lush illustrated cartoon mangrove art. Create ONE full bleed landscape 1536x1024 illustration, no panels or text. Brazilian mangrove lagoon with turquoise reflective water, gnarled mangrove trees and exposed roots on left and right, layered tropical canopy, distant hazy teal jungle, sunlit cyan sky and warm rays. Rich detailed painterly game art with dark confident contours and saturated yellow-green leaves, beautiful depth. Camera side-on. Water occupies bottom 25 percent and continues to bottom edge. NO foreground ground platform: that is rendered separately in game. Open clear center for gameplay. No crocodile, no characters, no coins, no obstacles, no interface, no logos. Left and right edges similar colors for scrolling repeat.

### Vila

Production side scrolling game background, ONE landscape 1536x1024 full bleed image, same rich outlined hand painted cartoon art as attached JacaRun reference. Brazilian tropical fishing village on wooden stilts, colorful turquoise coral yellow houses with terracotta roofs, fishing nets and little boats in turquoise water, palm trees, sunny blue sky, distant coast, warm adventurous mood. Side-on view, buildings in mid-distance, open water in lower 25% continuing to bottom, no foreground running platform (separate game layer). Composition readable behind a side-scrolling character, lush details at edges. No text, no labels, no panels, no characters, no coins, no UI, no logo.

### Floresta

Production side-scrolling game background, ONE landscape 1536x1024 full bleed image, match lush richly shaded outlined cartoon art of attached JacaRun reference. Brazilian tropical rainforest with tall ancient trees, twisting vines, layered giant emerald leaves, red heliconia flowers, sun shafts through canopy, turquoise waterfall cascading in center distance into a luminous lagoon. Dense enchanted but cheerful jungle, beautiful saturated greens, teal atmospheric depth, warm golden highlights. Side-on view. Open lagoon water lower 25% continuing to bottom edge. No foreground ground platform as separate game layer. No characters, no coins, no obstacles, no text, no panels, no logo or UI. Wide background for side scrolling mobile platform runner.

### Piso

Game asset sheet for JacaRun matching reference's beautiful outlined shaded cartoon. Transparent PNG background. Landscape 1536x1024. Exactly THREE horizontal ground platform strips, each spans full width left to right, each around 240 pixels tall, separated by 90 pixels transparent space. Top strip: golden sandy trail on top, grass tufts along rim, rich brown cross-section soil with stones and dangling roots under edge. Middle strip: wooden fishing pier, golden planks horizontal top surface, dark timber structure below, rope and wooden support posts beneath only. Bottom strip: lush tropical jungle ground, yellow ochre trail on top, bright green grass rim, mossy stone and roots cross section. All platforms have flat WALKABLE TOP at same height across entire width, side view, no perspective vanishing point, left and right edges repeat seamlessly. No characters, no obstacles atop, no text, no backgrounds, no UI. Top strip contained y=70..300; middle y=400..630; bottom y=730..960. True transparent surroundings.

### Objetos

Production game object atlas matching attached JacaRun lush outlined cartoon art. Transparent PNG 1536x1024, strict 4 columns x 2 rows of equal 384x512 cells. One isolated item centered per cell with generous transparent margin, no overlap. Top row left to right: gnarled mangrove stump/root obstacle; horizontal cut fallen tree log with visible rings; chunky moss-covered gray boulder; horizontal overhead branch with leaves hanging from short vine. Bottom row left to right: cheerful red crab with big eyes and claws; turquoise jumping fish facing LEFT; golden orange rare fish facing LEFT; shiny golden coin with embossed star. Polished rich shaded illustration, thick dark outlines, readable small game sprites. No words, no labels, no grid, no ground, no colored backdrop. Keep objects fully within each cell, isolated transparent alpha.
