# Prévia cooperativa local: retrato × paisagem

PROTÓTIPO — não instalado. Material de comparação visual e replay de estados do harness; não é firmware, integração de jogo ou medição de placa.

## Entregas

- `output/coop-preview/coop-portrait-2x.png`: duas capturas de 240 × 320 ampliadas exatamente 2×, onda e chefe.
- `output/coop-preview/coop-landscape-2x.png`: os mesmos dois estados em 320 × 240, ampliados exatamente 2×.
- `output/coop-preview/coop-turnos-replay.gif`: todos os 27 turnos confirmados, com ambas as orientações. Duração ilustrativa de 1 segundo por ação; 27 segundos no total. Não representa ritmo, latência ou FPS reais de hardware.
- Quatro PNGs nativos, manifesto de fontes/arte/hash, resultados das verificações e o trace congelado no pacote.

As capturas estáticas são as revisões 3 e 25. Na revisão 3, Goblin acaba de agir e Lobo é o próximo. Na 25, Guardião acaba de agir e J1 é o próximo. HP/MP/guarda e ordem vêm dos respectivos pós-estados confirmados. `D` indica defesa ativa. `OSS` é a abreviação visual do Esqueleto (espécie 2); não é um novo inimigo.

## Arte reaproveitada

O renderer lê o `artes.pak` existente de `clock9-release-work`, valida envelope PKA1, tamanho, contagem e CRC32, e usa o catálogo correspondente. Não gera imagens novas por IA e não modifica nenhuma arte original.

- Fundo: `BackgroundArt` IDs 58/59, recorte central preservando proporção, sem esticar. O enquadramento varia conforme a proporção da área disponível.
- Jogadores: `RaceArt` + máscaras de roupa + conversão RGB565 equivalente a `PersonalArt`. Usa os valores reais de raça, classe, camisa e calça do perfil sintético no trace. Não acessa saves do usuário.
- Monstros e Guardião: `EnemyArtCatalog`/codec indexed4; os recortes fixos da união de poses vêm de `EnemyScale`. Verificado que incluem exatamente toda a silhueta das quatro poses das cinco espécies usadas.
- Molduras: nove fatias do `assets/ruins7/ui-frame.png` existente.
- Fonte da tela: o bitmap `glcdfont.h` do próprio jogo. Textos explicativos fora das telas usam DejaVu Sans.

Todos os IDs lidos, arquivos de origem, hashes SHA-256 e versões de Python/Pillow estão no manifesto. Escalas dos sprites são propostas visuais, sem alterar o tamanho ou conteúdo dos arquivos de arte.

## Separação do estado e da apresentação

O trace é lido uma vez. Não há simulação de dano no renderer, RNG, gravação de campanha, pacote de rádio, publicação ou chamadas ao firmware. Alterar tamanho/posição/pose não altera HP, MP, alvo, ordem ou hash.

Cada linha contém o estado após uma ação confirmada. Em transições de onda, o renderer mostra o novo estado e a mensagem de conclusão da onda anterior, sem animar um ataque sobre um inimigo substituto. Um alvo derrotado pode permanecer brevemente translúcido durante a pose de impacto, com HP 0, e desaparece no repouso. Em estado terminal não se apresenta outro ator como próxima vez. O hash terminal é 180348654.

A ordem é a rotação dos slots vivos 0, 1, 3, 4, 5, iniciando em `next_actor_id`. O slot 2 é vazio em todas as 27 linhas. A legenda `J3: RESERVA FUTURA` só expõe essa reserva técnica; não significa terceiro jogador implementado nem comprova um layout final para três pessoas.

`Foco*` é a ação provisória do harness (2 MP, +3 dano), não uma habilidade final de classe. Defesa também é a regra demonstrativa do harness. As quatro zonas de controle apresentam Atacar, Foco, Defender e Itens; Itens apenas representa a futura escolha entre as duas ações já existentes no modelo, poção HP/MP. Não há menu ou controlador de toque implementado.

## Verificações realizadas

- 27 linhas sequenciais, seis slots por linha, dois jogadores e reserva vazia; terminal esperado.
- 216 renders nativos (27 estados × 4 poses × 2 orientações) com assert de limites para texto, painéis e sprites.
- Quatro zonas de controle em cada orientação, limites válidos, sem sobreposição entre elas ou com status/evento.
- Classificação exaustiva dos 153.600 pontos das duas telas, sem ponto pertencendo a duas zonas de botão.
- Retrato: quatro botões de 114 × 36 pixels. Paisagem: quatro de 75 × 30. São geometrias propostas, sem prova de conforto de toque físico. A largura adicional permite mais separação lateral; a altura menor aperta o HUD e os botões.
- Ambas as telas têm 76.800 pixels e um framebuffer RGB565 completo exigiria 153.600 bytes. Isso é aritmética de área, não RAM livre, pico, orçamento final ou desempenho do ESP32.
- GIF final reaberto, dimensões verificadas e duração total conferida: 27.000 ms. Há 108 quadros gerados, comprimidos em 67 quadros codificados porque poses idênticas são agregadas pelo codificador.
- Inspeção visual das duas folhas e de amostras do GIF, inclusive transições de onda e terminal.
- Scripts passam `python -m py_compile`.

Não foram executados firmware, suites integrais, rádio, dispositivos, orientação de touch, FPS, profiling de RAM/flash ou publicação. Nenhuma orientação é escolhida automaticamente.

## Reproduzir

Na organização de trabalho usada aqui, manter os scripts em `coop-prototype-work/prototypes/cooperative_preview`, o checkout somente-leitura `clock9-release-work` ao lado e o trace em `output/coop-prototype/turn-trace.jsonl`. Executar, na raiz do workspace:

    python coop-prototype-work/prototypes/cooperative_preview/render_preview.py

Requer Python 3 com Pillow já instalado. O pacote de revisão não duplica o atlas de 7,47 MB; a reprodução requer as fontes correspondentes aos hashes do manifesto. Os PNGs/GIF incluídos são abertos normalmente sem esse checkout.
