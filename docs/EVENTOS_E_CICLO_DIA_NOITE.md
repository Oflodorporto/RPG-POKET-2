# Eventos da Guilda e ciclo de dia/noite

**Plano e acompanhamento — 07/10/2026.** Conectado ao cânone `LORE_CANONICA_RPG_POKET_2.md`. A narrativa inicial foi integrada em lore1. A entrega dia1 implementa hora/fuso/ajuste manual e paletas de período, documentados em HORA_E_DIA_NOITE.md. Eventos, persistência diária, pergaminho e novos inimigos ainda são projeto posterior; o restante deste documento descreve o objetivo completo.

## Duas oportunidades por dia, oito histórias possíveis

Cada personagem recebe duas ofertas diárias distintas, sorteadas de um catálogo de oito eventos. Janelas padrão: uma oferta entre09h–12h e outra entre18h–22h, com minuto sorteado. Sorteio, variante e horário são persistidos por personagem/data local. Reabrir a tela ou reiniciar não muda o resultado. Recusar consome a oferta daquele dia; não permite sortear outra.

As janelas dão ritmo, mas não exigem deixar a placa ligada: ao voltar depois de um horário perdido, a oferta fica na caixa de cartas. As duas ofertas do dia atual podem ser vistas em sequência. Não acumular dias desligados. Não prometer disparar evento enquanto a placa está sem energia.

Aplicar uma seleção com variedade por personagem: não repetir o mesmo evento entre as duas ofertas; evitar os tipos dos últimos três dias quando houver opções. Os oito eventos possuem versões para faixas1–4,5–9,10–17 e18+, com inimigos, objetivos e recompensas adequados. A recomendação das cidades continua valendo; variantes iniciais ocorrem na região segura correspondente e não enviam um novato a Aurora. Se uma variante não puder ser implementada com segurança, oferecer outra elegível, sem reduzir o mínimo diário enquanto houver duas disponíveis.

Todas as ofertas indicam região, dificuldade, tempo previsto e recompensa garantida. Eventos de socorro da Guilda são acessíveis sem filiação; contratos comuns e Clube da Luta continuam seguindo suas regras. Ter uma missão da Guilda ativa não a sobrescreve. Aceitar só quando o herói estiver em estado seguro e comHP>0; nunca cancelar um combate, viagem ou dungeon silenciosamente.

## O pergaminho acima do relógio

A “tela de descanso” deste pedido é o **relógio de inatividade**, que já aparece após um minuto. Não é a animação de acampamento de4,5s. Com uma carta disponível, exibir um ícone de pergaminho acima da hora, pulsando suavemente a cada900ms. O fundo permanece preto e o brilho reduzido. Duas cartas usam um pequeno contador, sem sobrepor data e horário.

Ao tocar, a primeira ação abre o pergaminho; não aceitar automaticamente. A abertura dura aproximadamente450ms, com animação local leve, sem acessar o cartão a cada quadro. Voltar ou fechar mantém a carta disponível. Uma carta também pode ser acessada em Menu > Cartas, para quem desativou o relógio ou está jogando quando ela chega.

Distribuição sugerida na tela240×320: remetente e selo no topo; nome da missão e descrição no centro; recompensa, risco e duração acima do rodapé; **Aceitar** e **Recusar** em botões grandes. Conteúdo longo usa páginas com “1/2”; decisão permanece fixa no rodapé. Após Recusar, pedir confirmação curta para evitar perda por toque acidental. Aceitar uma carta de risco elevado também oferece confirmação clara.

Notificar discretamente fora do relógio; nunca abrir sobre combate, download, rolagem, viagem ou acampamento. Enfileirar até o estado seguro. Mudar o horário ou perder Wi-Fi não pode travar as animações.

## Catálogo de oito eventos

XP e ouro abaixo são valores-base de projeto por faixa, sujeitos a testes de economia: I=1–4;II=5–9;III=10–17;IV=18+. Não representam o balanceamento já entregue. Recompensa do contrato é paga uma única vez ao concluir; quando houver recompensas normais de combate, o total esperado deve ser mostrado e balanceado, sem pagamento duplicado do contrato.

| ID / tipo | Missão e remetente | Objetivo e variação | Ouro / XP por faixa I,II,III,IV |
|---|---|---|---|
| ev_hipogrifo / combate | **Asas sobre os telhados**, Maelis Voss | Um hipogrifo desorientado pelo farol ameaça a cidade. I: um jovem no pomar;II: adulto nas Ruínas;III: hipogrifo marcado na costa;IV: alfa sobre Aurora. Combate com opção narrativa de afugentar após vencer; não matar automaticamente na conclusão. | 25/20;70/60;150/140;300/280 |
| ev_ervas / coleta | **A última luz do bosque**, Elarin | Reunir três ervas-lanterna antes que percam o brilho. I: borda do bosque;II: bosque e antigo marco;III/IV: plantas raras em áreas adequadas. Objetivo por interações claras, até dois encontros, sem exigir sorte infinita em drops. | 15/15;45/45;100/100;210/220 |
| ev_cripta / dungeon | **O sino sob a pedra**, Iria | Um sino enterrado atrai mortos. I: pequena adega de duas salas, não a Cripta8+;II: limpar um setor permitido das Ruínas;III/IV: expedicões maiores. Ingresso custeado pela Guilda, sem gastar cristal pessoal; estado da dungeon comum preservado. | 30/30;90/90;190/190;380/380 |
| ev_mercador / escolta | **A estrada ainda é nossa**, Tomás | Conduzir um mercador e sua carga a outra região. I: trajeto curto perto de Carvalho;II: Carvalho–Ruínas;III: rota até Mares;IV: Mares–Aurora. Travessia comD20 e uma ou duas ameaças, bônus de sobrevivência aplicável; carga tem indicador próprio. | 30/25;80/75;170/165;330/320 |
| ev_farol / reparo | **Uma luz para quem volta**, Nilsa | Recuperar peça de lente e religar um marco/farol. Coleta curta seguida de defesa contra uma onda de nível adequado. Revela uma pista do Caminho sem antecipar os segredos do atoIII. | 25/25;70/70;145/150;290/310 |
| ev_caravana / resgate | **As rodas na neblina**, Sabela | Encontrar uma caravana perdida, enfrentar emboscada e trazer sobreviventes ao posto seguro. A identidade da ameaça varia com região e horário. Falha nunca vira morte arbitrária permanente de um NPC central. | 25/25;75/70;160/160;320/330 |
| ev_selos / investigação | **Nomes gravados em cinza**, Seraphine | Examinar três selos, ordenar símbolos e enfrentar o eco que os corrompeu. Sem puzzle impossível por tela pequena; pistas e opção de tentar novamente. VersãoI usa inscrição simples de Carvalho; demais seguem áreas conhecidas. | 20/25;65/75;140/170;280/350 |
| ev_acampamento / defesa | **Uma chama contra a noite**, Grum | Proteger um acampamento de viajantes por duas ondas curtas ou reparar suas defesas. Mais comum à noite, mas variante diurna serve para não excluir jogadores por horário. Ao concluir, alimento oferecido pelos viajantes é mostrado como recompensa adicional definida, não cura oculta. | 25/20;70/65;150/150;300/300 |

Todos são acontecimentos ligados ao enfraquecimento dos faróis, mas nem todo texto precisa revelar a trama principal. NPCs centrais não morrem em eventos repetíveis. Missões de coleta têm itens de missão separados dos consumíveis pessoais; limpar dungeon tem inimigos finitos e um encerramento inequívoco; escoltas mostram origem, destino e como retornar.

### Carta completa — Asas sobre os telhados

**Selo: Guilda dos Aventureiros — Maelis Voss**

“Caro aventureiro,

Um hipogrifo começou a sobrevoar Carvalho. Já atingiu os telhados do mercado, e nossa guarda está ocupada protegendo a estrada. A ave parece assustada; há uma luz estranha nas penas.

Soube que você está pelas redondezas. Precisamos de alguém que a afaste antes que uma família se machuque. Você será recompensado, é claro. Se houver uma forma de poupá-la, tente. Algo a trouxe até nós.

Conto com você,
**Maelis Voss**
Mestra da Guilda dos Aventureiros.”

**VarianteI:** Pomar de Carvalho • jovem hipogrifo •25ouro e20XP • cerca de2–4min • sem taxa de entrada. Botões: **Aceitar / Recusar**. A carta varia o nome da cidade e a ameaça conforme a variante; não enviar a mesma descrição de Carvalho para uma missão em Aurora.

Ao aceitar, salvar primeiro a missão e o ponto de retorno. Mostrar uma transição curta no mapa: “A Guilda providenciou sua passagem.” Abrir diretamente a cena da missão, como solicitado, sem viagem comum ou custo oculto de ração. A história evita um teleporte mágico gratuito em cada carta. O herói volta à região e à tela segura de origem após concluir, desistir ou perder, mantendo as consequências de HP/MP conforme as regras anunciadas. Escoltas seguem o trajeto durante a missão, mas usam esse retorno ao final para não abandonar o jogador longe do ponto de partida.

## Hora local e mudança do mundo

| Período | Hora local | Apresentação | Atividade |
|---|---|---|---|
| Amanhecer | 05:00–06:59 | Luz dourada e neblina leve | Retorno de viajantes; criaturas noturnas recuam |
| Dia | 07:00–17:59 | Paleta atual clara | Mercado, caravanas e ameaças diurnas |
| Crepúsculo | 18:00–19:59 | Luz âmbar, tochas começando a acender | Guarda reforça estradas; variedade de encontros |
| Noite | 20:00–04:59 | Cenário escuro legível, lua e luzes locais | Ecos do Véu, neblina e criaturas noturnas |

Configurações oferecem fuso local, hora automática e ajuste manual. Padrão inicial compatível com o usuário: América/São_Paulo,UTC−3; não confundir horaUTC com a hora da cidade do jogador. Sincronização de rede é periódica e não bloqueia o jogo. Sem internet, continuar usando a última hora válida enquanto ligado; em reinício sem fonte de tempo confiável, informar “Hora não sincronizada” e permitir ajuste manual. Não inventar uma data nem prometer que a placa mantém hora desligada sem verificar um RTC utilizável.

Uma mudança de período acontece fora de combate ou ao terminar a cena atual. O inimigo de um encontro iniciado às17:59 não se transforma durante a luta. Missão e encontro guardam a variante sorteada para retomada. A dungeon normal conserva sua posição e inimigos; horário altera atmosfera e futuros encontros elegíveis, não ressuscita os já vencidos.

### Criaturas por região

| Região | Dia | Noite | Progressão e sentido narrativo |
|---|---|---|---|
| Carvalho | Javali, lobo, saqueador de estrada iniciante | Lobo das brumas, mariposa de cinza, eco do bosque | A Raiz perde sua proteção; animais fogem de algo maior. |
| Ruínas | Esqueleto, vigia ossudo e saqueador de relíquias | Espectro, sombra de vigília e esqueleto desperto | Memórias presas se manifestam com mais força à noite. |
| Mares | Saqueador, contrabandista e caranguejo de couraça | Afogado do Véu, saqueador de neblina e espectro de marinheiro | Rotas e marés ligam comércio ao desaparecimento de tripulações. |
| Aurora | Sentinela, desertor e guardião de estrada | Sentinela sem rosto, cavaleiro da vigília e eco do juramento | A defesa real começa a se separar das pessoas que deveria proteger. |

Nomes novos acima são propostas de inimigos e assets, não novosIDs já existentes. A noite muda o elenco e pode elevar modestamente o perigo, mas precisa manter encontros adequados ao nível e avisos visíveis. Missões principais não exigem jogar de madrugada; oferecer preparação/esperar até a noite por transição de jogo sem mudar o relógio real nem gerar ofertas diárias extras.

Para o displayLCD, escurecer o fundo não reduz sozinho a iluminação: economia de energia depende do brilho da luz de fundo. Preservar o relógio preto com brilho baixo; no jogo noturno manter texto, alvos e botões legíveis. Usar paletas, sobreposições e quadros preparados em memória; não consultar SD ou servidor durante cada atualização da tela.

## Persistência e integridade

Registro por personagem: data das ofertas, dois IDs/variantes, estados pendente/recusado/aceito/concluído, oferta ativa, progresso, recompensa paga, região/tela segura de origem e histórico recente. Não encaixar dados novos nos bytes90/91 do save10, já usados pelo acampamento. Desenhar uma extensão ou registro separado com migração e diário antes de programar.

No começo de um dia, sortear e salvar a agenda uma vez. Voltar o relógio não reabre dias já processados. Avançar muito o relógio concede apenas ofertas da data atual, sem acumular recompensas. Um contador monotônico/data máxima observada evita duplicação básica, mas relógio manual e dispositivo offline não garantem proteção contra fraude deliberada; não prometer isso para o futuro comércio multiplayer.

Após aceitação, manter a missão ativa entre dias e reinícios. Não expirar um combate às00h. Ofertas não aceitas podem ser substituídas no dia seguinte, com aviso; missão aceita tem prioridade e as novas ofertas aguardam na caixa. A lista não sobrescreve contratos existentes. Recompensa, progresso e retorno são transações verificadas, não efeitos da animação. Falha de save pausa a transição e permite repetir sem duplicar ouro,XP ou itens.

## Próxima sequência de trabalho

1. Reorganizar diálogos, serviços e mapa usando o cânone; introduzir NPCs e diário sem alterar os IDs atuais nem apagar progresso.
2. Projetar persistência narrativa/eventos e migração dos três slots; reconhecer guardião e cripta já concluídos.
3. Implementar relógio/fuso e paletas de período, mantendo animações e toque responsivos.
4. Implementar duas ofertas diárias, pergaminho e carta do hipogrifo como evento piloto completo; testar aceitação/recusa/reinício/retorno/recompensa única.
5. Completar os outros sete eventos e as variantes de nível/dia/noite, com assets e balanceamento; publicar somente após testes, compilação e inspeção visual.

Esses passos substituem a prioridade anterior de desenvolver alcance como próxima tarefa. Alcance continua pendente e deve ser encaixado quando combate das novas missões exigir, sem declarar a mecânica pronta.
