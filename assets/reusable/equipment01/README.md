# Equipamentos01 — cópia compacta para o repositório

Esta biblioteca conserva somente PNGs nativos de32/48 pixels, props, miniaturas
de mapa, atlases e coordenadas/metadados. Não é carregada automaticamente no
firmware. Somente a tenda aprovada27×20 é convertida por
`tools/prepare_tent_marker.py` para1.080 bytes RGB565 constantes na flash,
sem ampliar o artes.pak nem reservar PSRAM.

Os22MB de imagens de geração, prompts.json, build_pack.py, fontes completas,
prévia/QA e ZIP ficam na entrega integral da Library. Não estão duplicados
no repositório. Os PNGs nativos aqui são as fontes autoritativas para futura
integração; as coordenadas e tamanhos estão no manifest.json.

O texto original do pacote integral segue para preservar as instruções de
uso. Referências a sources, prompts, build_pack e QA abaixo pertencem
somente ao ZIP completo entregue separadamente.

---

# RPG POKET — Equipamentos e Acampamento v1

Pacote artístico reutilizável criado em 13 de outubro de 2026 para o RPG POKET, em fantasia medieval detalhada de 16 bits. Todos os objetos foram gerados individualmente com imagegen, com transparência verdadeira. Os PNGs nativos foram recortados, reduzidos proporcionalmente e organizados por script, sem alterar o firmware.

## Conteúdo

- 24 peças diferentes em `png32/` (32×32) e `png48/` (48×48).
- 8 variantes maiores para cenas em `props/`, com alinhamento ao chão e âncoras no manifesto.
- 8 miniaturas de reserva em `map/`: barraca em 23×16, 27×20 e 24×24; rolo, mochila, lanterna, tripé e lenha em 24×24.
- Dois atlas transparentes em `atlases/`, cada um acompanhado por JSON de coordenadas.
- 24 originais de alta resolução em `sources/`.
- Prancha de apresentação `POKET_equipamentos_acampamento_preview.png`, com rótulos fora das artes.
- `manifest.json` com nomes estáveis, dimensões, caminhos, âncoras, bounds e relação com itens existentes.
- `prompts.json` com os prompts de origem; `build_pack.py` reproduz as exportações (Python 3, Pillow e NumPy).
- `qa/validation.json` registra verificações de dimensões, transparência, margens e hashes únicos.

## Peças

Barraca de lona, kit de dormir enrolado, saco de dormir, mochila, corda, lanterna, tocha, panela, tripé de cozinha, lenha, ração de viagem, odre, mapa, bússola, sucata, kit de bandagens, kit de reparos, cajado de carvalho, espada de ferro, lança de ferro, machado de ferro, colete de couro, amuleto simples e escudo.

Os kits são agrupamentos visuais únicos, como ferramentas presas em um estojo. Cada arquivo de inventário contém somente sua peça ou kit completo, sem duplicação, legenda, painel ou interface embutidos.

## Uso no jogo

Use `png32/` em listas e inventário compacto; `png48/` para destaque ou detalhes. Os sprites preservam no mínimo 2 pixels de margem nas caixas de inventário. Para aumentar o desenho, use **nearest-neighbor / vizinho mais próximo**. Evite filtragem bilinear e não estique proporções.

O atlas de 32 px mede 216×144; o de 48 px mede 312×208. São 6 colunas por 4 linhas, com 2 px de margem transparente em cada lado da célula. As coordenadas JSON são relativas ao canto superior esquerdo. Cada `frame` inclui a caixa quadrada completa do sprite; não há rotação nem trim oculto. Use os nomes em inglês como chaves estáveis; os rótulos em português são apenas descritivos.

Todos os arquivos em `png32/`, `png48/`, `props/` e `map/` são RGBA de 8 bits com **alpha binário 0/255**, sem fundo colorido ou quadriculado queimado. Os originais em `sources/` preservam o alpha gradual da geração. A exportação descarta somente resíduos quase invisíveis de alpha até 16 antes do recorte, limita a paleta a 48 cores por sprite e remove pixels externos transparentes. Não substitua preto por transparência: o contorno e a entrada da barraca usam tons muito escuros legítimos.

Para RGB565, converta somente os pixels opacos e preserve a máscara alpha à parte, ou escolha uma chave de transparência garantidamente ausente na paleta. Este pacote não impõe uma chave RGB565 e não foi incorporado automaticamente ao `artes.pak`.

## Props e mapa da viagem

A barraca maior mede 96×80 e mostra entrada aberta, painel lateral, cumeeira, cordas e estacas. Os props são alinhados ao chão, com o apoio escuro da própria arte; o `anchor` indica o ponto de posicionamento no chão. Não incluem um piso retangular nem cenário.

As barracas pequenas são propostas reutilizáveis para o mapa da viagem. A versão 27×20 preserva mais detalhe; a 23×16 economiza espaço. Verifique a composição na tela real de 240×320 antes de integrar: a miniatura deve ficar afastada de nomes de cidade, linhas de rota e indicador do jogador. As outras miniaturas de mapa são reservas e não acrescentam funcionalidades por si.

## Relação com o catálogo atual

O rolo e o saco de dormir são duas aparências do mesmo `sleepKit`, e não dois itens novos. A ração corresponde a `rations`, o mapa a `charts` e a sucata a `scrap`. As seis famílias de equipamento correspondem às entradas básicas de `Gear.h`: cajado 1, espada 4, lança 7, machado 10, armadura 13 e amuleto 16.

Os demais objetos são **reserva artística para uso futuro**. O escudo, a bússola, o odre e os kits não recebem estatísticas, preços, regras ou slots novos neste pacote. Nenhum arquivo de firmware, catálogo de arte ou release foi modificado.

## Limites e verificação

As imagens são estáticas. A chama da tocha e a luz da lanterna estão presentes no próprio objeto; animações e efeitos independentes pertencem ao pacote separado de efeitos. A arte foi inspecionada visualmente em conjunto e nas dimensões finais, e as margens/alpha/hashes foram conferidos automaticamente. Miniaturas extremamente pequenas simplificam os detalhes; não representam uma promessa de legibilidade de todo detalhe do original.

Os originais são imagens geradas por IA para este projeto, com direção medieval própria, sem marcas ou personagens licenciados solicitados. Não há bibliotecas de arte de terceiros incluídas.
