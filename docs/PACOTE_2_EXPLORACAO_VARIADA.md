# Pacote 2 — Exploração variada

Versão 2026.10.08-exploracao1, build 2026100807. Candidata em validação; não recomendar instalação antes de verificar a release final em CONTINUE_ATUAL.txt.

Explorar agora sorteia 50% combates, 20% achados, 20% baús e 10% pequenos encontros positivos na estrada. Guardião, missões de campanha, viagem D20 e dungeon conservam seus acessos e controles.

Achados e baús podem dar ouro, vida, mana, ração, bota velha, roupa rasgada ou arma da classe. Ouro varia pela região; armas respeitam nível e região. Equipamento já possuído converte para ouro explicitamente na tela de recibo. Limites da bolsa não apagam a descoberta: recolher pode ser repetido ou o achado pode ser deixado. As descobertas não concedem XP; combater continua sendo a progressão principal.

Baús oferecem Abrir/Deixar. Cinco por cento dos baús escondem mímicos, cerca de um por cento de todas as explorações. Um ruído indica risco antes de abrir. Cada região tem seu mímico; Carvalho reduz HP/ataque para heróis novos de nível1–2. A criatura recebe uma silhueta de baú e animação de mandíbula própria, construída no renderer. Vitória dá os prêmios normais do combate, uma vez; não permite abrir outra vez o mesmo baú. Derrota/fuga deixam o encontro e retornam ao contexto normal de exploração/recuperação.

Sucata tem pilha separada de até9; não ocupa espaço de cristais ou poções. Bolsa > Sucata permite vender a pilha por2 ouro/unidade ou descartar. A transação e o recibo são persistidos, sem conceder novamente após reinício.

As telas acompanham o ciclo de iluminação. Dia/noite altera a provisão dos encontros locais quando o relógio e o ciclo estiverem ativos. Essa entrega não inclui oito eventos diários, novos mapas, gazuas ou fechaduras; esses permanecem nos pacotes posteriores.

Save18 mantém128 bytes e CRC124. Bytes60–63 (não usados nos saves modernos anteriores) guardam estado da descoberta, conteúdo, quantidade e sucata. Lê formatos1–18 e conserva valores antigos; campos novos começam zerados. Descoberta é salva antes da tela; recibo antes de continuar. Falha de gravação pausa para repetir o mesmo save, sem sortear nem premiar novamente. Firmware anterior não lê save18: não fazer downgrade após salvar sem backup compatível.

Artes existentes conservadas, sem aumento do pacote SD nem leitura a cada quadro. Não houve flash de placa, .rpg, formatação ou limpeza de saves.

## Teste físico

1. Atualizar na placa após confirmação da publicação, abrir um herói existente e conferir seus recursos.
2. Explorar em Carvalho e Ruínas por uma sessão curta: alternar combate/achado/baú/encontro, abrir ou deixar baús.
3. Reiniciar em descoberta e em recibo: conteúdo e prêmio devem permanecer; continuar não premia outra vez.
4. Conferir cristais/poções/equipamento na bolsa; tocar Sucata, vender ou descartar.
5. Observar o mímico quando aparecer naturalmente; conferir batalha e retorno, mapa/D20 e dungeon. Não precisa procurar por repetidos reinícios: não rerrola.

Próximo: Pacote 3 — Trancas e gazuas.
