# Valdária — atlas narrativo do RPG POKET

**Cânonev1 — 07/10/2026.** O continente se chama **Valdária**. O mapa atual é **Aeldra, as Terras da Primeira Aurora**, uma região do continente. Aeldra contém os destinos Carvalho, Ruínas de Véspera, Mares e Aurora. Na interface atual, manter o nome curto “Ruínas” e os quatroIDs existentes.

Escala: **continente → região com mapa próprio → cidade/local → dungeon**. “Mares” é uma cidade portuária de Aeldra; “Talassar” é a futura região marítima. Nenhum desses novos mapas está implementado na camp1.

## As oito regiões

| Região / nome evocativo | Bioma e posição | Identidade e conflito central | Destinos propostos |
|---|---|---|---|
| **Aeldra — Terras da Primeira Aurora** | Oeste temperado: bosque, campo, ruínas, costa e fortaleza | Reino dos quatro faróis. Memórias roubadas sustentam a paz; o herói precisa reconstruir o pacto. Primeira campanha do jogo. | **Carvalho**, **Ruínas de Véspera**, **Mares**, **Aurora**; nomes atuais conservados. |
| **Nivária — Coroa do Inverno** | Extremo norte: tundra, fiordes e gelo antigo | Clãs mantêm fogueiras que jamais podem se apagar. O inverno prende dias inteiros dentro de geleiras, e uma expedição desaparecida ainda envia sinais de uma semana que não terminou. | **Vigília Branca**, porto de **Skeld**, **Fenda Azul**, **Mosteiro da Última Chama**. |
| **Kharum — Montanhas que Cantam** | Cordilheira central, entre Aeldra e o interior | Cidades escavadas usam sinos para orientar túneis. As montanhas começaram a responder com vozes de mineiros mortos. A disputa é entre exploração, segurança e o que os próprios anões lacraram. | **Setepicos**, **Ponte do Martelo**, **Mina dos Ecos**, **Salão de Pedra Viva**. |
| **Silvaran — Floresta sem Fim** | Nordeste: floresta antiga, raízes gigantes e cidades suspensas | Trilhas mudam conforme lembranças de quem caminha. Comunidades precisam escolher como receber quem foge de outros reinos sem destruir sua própria proteção. Uma árvore cresce com nomes esquecidos. | **Lúmen das Folhas**, **Clareira dos Pactos**, **Raiz Profunda**, **Templo dos Cem Caminhos**. |
| **Veldrûm — Pântanos do Véu** | Sudeste: brejos, canais e aldeias sobre estacas | A água guarda rostos que nunca existiram. Curandeiros, barqueiros e investigadores tentam separar doença, fraude e manifestações reais do Véu. Autoridades querem queimar as aldeias para “limpar” a região. | **Vau das Lanternas**, **Porto Junco**, **Casa Submersa**, **Santuário dos Sem Nome**. |
| **Sahram — Mar de Vidro** | Sul interior: dunas, oásis e planícies de sal e vidro | Uma guerra antiga vitrificou parte do deserto. Caravanas disputam poços, mapas e ruínas soterradas; miragens revelam futuros possíveis. Uma cidade está negociando a própria chuva. | **Qadira**, **Oásis das Sete Sombras**, **Dunas de Âmbar**, **Observatório do Sol Partido**. |
| **Ignária — Terras da Brasa** | Sul junto à cordilheira: vulcões, basalto e forjas | A terra produz metais vivos e também cinza que sufoca colheitas. Ferreiros e comunidades de trabalhadores enfrentam senhores que querem transformar uma criatura primordial em combustível permanente. | **Fornalha Alta**, **Vale da Cinza**, **Ponte Escarlate**, **Coração de Basalto**. |
| **Talassar — Ilhas das Mil Marés** | Mar ocidental e sudoeste, sobre a plataforma de Valdária | Região insular e marítima: portos livres, recifes, tempestades e ruínas submersas. Cartas náuticas passam a mostrar ilhas que ainda não emergiram. Guildas comerciais, piratas e guardiões do mar disputam quem pode atravessar suas águas. | **Porto Velamar**, **Ilha da Maré Mansa**, **Recifes da Lua**, **Palácio Afogado**. |

As culturas não são monopólio de uma raça. Nivária não contém somente bárbaros; Kharum não contém somente anões; Silvaran não exclui humanos ou orcs. Cada região pode ter conflitos internos e aliados de origens diferentes.

## Geografia para desenhar os futuros mapas

Ao norte ficam Nivária e seus fiordes. Kharum atravessa o centro de norte a sul. Aeldra fica a oeste da cordilheira, e Silvaran a nordeste. Veldrûm recebe os rios do lado oriental. Sahram ocupa o sul interior, protegido das chuvas por Kharum. Ignária é a extremidade vulcânica da cordilheira, a sudoeste de Sahram. Talassar fica no mar a oeste/sudoeste, alcançável pelos portos de Aeldra e Ignária.

Conexões principais: **Mares → Talassar** por navio; **Aurora → Kharum** por passo de montanha; **Kharum → Nivária** ao norte; **Kharum → Silvaran** a leste; **Silvaran → Veldrûm** pelos canais; **Kharum → Sahram** por caravanas; **Sahram → Ignária** pela estrada do basalto; **Ignária → Talassar** por porto vulcânico. Estas conexões são geografia narrativa, não novas rotas já adicionadas ao código.

No futuro, Menu > Mapa abre primeiro o mapa da região atual. Um botão **Continente** mostra a visão geral e os portos/passagens descobertos. Tocar numa região abre seu mapa ou explica o requisito de acesso. Manter nomes curtos visíveis e informações em painel, sem diminuir os alvos de toque para encaixar todo o continente numa tela240×320.

## Campanhas e progressão

A primeira campanha termina em Aeldra e vale por si mesma. As expansões não dizem que “o verdadeiro vilão controlava tudo” a cada novo mapa. O Véu Oco pode conectar mistérios, mas cada região tem história, problemas humanos e solução próprios. Faróis são a resposta específica de Aeldra; Nivária usa fogueiras e gelo, Kharum usa ressonância, Sahram investiga sol e miragens.

As novas regiões serão acessadas conforme progresso e preparação, com missões introdutórias seguras e avisos de risco. Não fixar agora que todo Sahram seja um nível específico: o nível de entrada e os setores avançados serão balanceados com a evolução real do personagem, até o teto atual99. Uma região pode ter costa inicial, interior intermediário e dungeon avançada. Isso evita gastar oito mapas apenas como degraus iguais de inimigos com maisHP.

Viagens continentais ganham apresentação própria: navio em Talassar, trilha de montanha em Kharum/Nivária, caravana em Sahram, barca em Veldrûm. Custos, comida, D20 e encontros precisam ser apresentados antes de partir. Introduzir frio, sede ou navegação apenas quando houver equipamento, contrajogo e UI adequados; não punir automaticamente jogadores sem novos itens ao importar saves.

Maelis mantém a sede da Guilda em Aeldra. A organização terá filiais com representantes locais nas outras regiões; completar uma missão não exige voltar fisicamente à sede. A filiação já paga acompanha o personagem, e novas filiais não cobram novamente100ouro.

## Primeiros representantes das regiões futuras

| Nome | Papel | Gancho pessoal |
|---|---|---|
| **Yrsa Ventoquieto**, orc | Guarda-fogueira de Nivária | Recebe cartas do irmão desaparecido com datas que ainda não chegaram. |
| **Hadrik Nove-Sinos**, anão | Intendente do passo de Kharum e agente da Guilda | Consegue reconhecer cada mina pelo som, menos a voz que começou a responder de sua casa. |
| **Lethiel Ramosclaros**, elfa | Guia de Silvaran | Uma trilha devolveu a ela uma lembrança que pertencia a outra pessoa. |
| **Mira dos Juncos**, humana | Barqueira e curandeira de Veldrûm | Seu barco conhece um canal que desaparece dos mapas ao amanhecer. |
| **Samira Nahl**, humana | Cartógrafa das caravanas de Sahram | Seu melhor mapa mostra um oásis que secou há cem anos e está voltando a aparecer. |
| **Urok Brasa-Mansa**, orc | Ferreiro e representante dos trabalhadores de Ignária | Recusa vender uma liga extraordinária porque escuta uma criatura respirando dentro dela. |
| **Neris Salvento**, elfa | Navegadora e agente da Guilda de Talassar | Procura um porto descrito por três capitães que nunca estiveram no mesmo século. |

Esses NPCs são pontos de partida canônicos, sem fichas de combate ou assets já criados. Não substituir osNPCs de Aeldra por eles.

## Organização para implementação

O identificador canônico da região atual é `aeldra`; cidades atuais mantêm os IDs0Carvalho/1Ruínas/2Mares/3Aurora. Novas regiões usam IDs textuais estáveis: `nivaria`, `kharum`, `silvaran`, `veldrum`, `sahram`, `ignaria`, `talassar`. Destinos futuros pertencem à sua região; não reutilizar `Game.city` sozinho para apontar cidades de outros mapas.

Uma futura migração acrescentará região + destino local (ou um IDglobal de local com tabela), conservando todos os saves existentes em Aeldra. Descobertas, lojas, missões e eventos devem referenciar essa hierarquia. Dados do catálogo narrativo ficam separados do estado do personagem. Antes de mudar o save, projetar o formato e testes de migração; nenhum novo mapa foi aplicado ao hardware nesta etapa.

**Referências:** `LORE_CANONICA_RPG_POKET_2.md` define a campanha de Aeldra; `EVENTOS_E_CICLO_DIA_NOITE.md` define o catálogo inicial de eventos e os períodos locais. Expansões recebem catálogos próprios, sem tornar as oito cartas de Aeldra o único conteúdo do continente.
