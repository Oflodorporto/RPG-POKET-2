# Reorganização narrativa — 2026.10.07-lore1

Primeira integração da lore no jogo. Novos personagens passam por raça, classe e roupa, depois quatro quadros da abertura **As Cinzas da Primeira Aurora** e o guia de Nara. A abertura permite avançar ou pular. Sua posição é de apresentação: reiniciar antes de concluir o guia volta ao começo da abertura, sem cobrar recursos. Personagens que já concluíram o tutorial não são obrigados a repeti-lo. Combates, viagens, acampamentos e dungeons em andamento continuam tendo prioridade na retomada.

**Menu > Diário:** seis páginas da campanha, abertas conforme as descobertas que o save atual pode comprovar. O tutorial, encontros nas Ruínas, Guardião vencido e conclusão da Cripta são reconhecidos. Livro das Vigílias é uma descoberta narrativa após derrotar Vaelor; não um item físico aleatório do baú. Páginas futuras permanecem ocultas, sem revelar o antagonista. Relembrar permite rever a abertura sem mudar personagem ou tutorial.

**Menu > Pessoas:** três personagens locais por destino, com nome, função e diálogo. Nas cidades, Conversar abre a mesma lista; nas Ruínas, toque em Iria Sorel acima da entrada da dungeon. Vaelor só revela seu diálogo após sua derrota. Maelis é identificada na Guilda e contratos; Grum no Clube. Forja, suprimentos e poções mostram seus responsáveis locais. Seus serviços atuais, preços, requisitos e taxas continuam iguais.

**Mapa de Aeldra > Continente:** atlas de Valdária, oito regiões e suas conexões narrativas. Aeldra contém os quatro destinos atuais; cada destino mostra o farol correspondente. As outras sete regiões estão em preparo e não alteram a posição do herói. Não foi desenhado um novo mapa continental ilustrado nesta entrega: o atlas usa páginas informativas sobre o fundo existente.

Refúgio das Brasas e Nara pertencem a Carvalho. Abrigos acessados a partir de outros destinos recebem a identificação local, evitando afirmar que o personagem foi transportado a Carvalho. Ruínas passa a se apresentar como Ruínas de Véspera. Vitória do chefe identifica Vaelor e menciona o Livro; o baú conserva seu sorteio de armas/ouro.

## Persistência e limites

**Save10,96bytes,sem migração nova.** O Diário é uma leitura dos campos existentes, não uma nova missão que paga XP ou ouro. Conversar, ler, rever e navegar no atlas não salvam nem alteram HP/MP, inventário, taxas, rolagens ou contratos. Os três slots permanecem separados. Não há leituras do cartão durante essas páginas; os fundos existentes já estão em memória.

Esta etapa organiza a apresentação e introduz a campanha. Ainda não implementa a restauração dos faróis, os atosIII–V completos, o chefe final Odran, as duas cartas diárias, dia/noite, novos inimigos ou mapas. Esses sistemas exigirão persistência e migração próprias. O alcance da dungeon continua adjacente. Os bytes90/91 continuam pertencendo ao acampamento.

## Verificação

Controlador real do sketch incluído no teste nativo: criação/abertura/pular/guia, retorno da abertura, navegação Diário/Pessoas/atlas, desbloqueio por progresso e ausência de escrita nos saves ao ler. O renderizador verifica os limites do texto e produz prévias de abertura, NPCs de quatro cidades, Diário de novato/veterano e oito regiões. Conferir a validação final em `validacao/lore1-ci.json` e `validacao/lore1-release.json` após a publicação.

## Teste na placa

1. No personagem existente, abrir Menu > Diário: o Guardião/Cripta já vencidos devem ser reconhecidos, sem repetir tutorial.
2. Rever a abertura e voltar ao Diário. HP,MP,ouro,itens e contratos devem permanecer iguais.
3. Abrir Pessoas em cada cidade; voltar ao local correto. Visitar Iria nas Ruínas e Vaelor se a cripta já foi concluída.
4. Conferir nomes dos responsáveis na Guilda, forja e poções; comprar somente se quiser, pelas taxas atuais.
5. No mapa, abrir Continente: navegar pelas oito regiões e voltar a Aeldra sem viajar para regiões futuras.
6. Criar um personagem somente em slot vazio: raça/classe/roupa, abertura, pular ou avançar, guia e abrigo. Não excluir um save para testar.

Nenhuma placa foi gravada automaticamente. Atualização deve ser confirmada pelo usuário na placa, ou feita manualmente pelo cabo. Sem arquivos.rpg, formatação, mudança de partições ou alterações na versão Heltec.