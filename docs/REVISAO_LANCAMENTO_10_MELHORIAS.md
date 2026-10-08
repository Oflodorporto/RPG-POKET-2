# Segunda revisão para lançamento — 8 de outubro de 2026

Diagnóstico de direção: alpha jogável com uma identidade visual forte. Ainda não recomendo anunciar um lançamento comercial completo. A campanha interrompe a história após a Cripta, a curva 1–20 não foi medida com jogadores e a primeira impressão precisa de teste externo. Mais imagens e cidades não resolvem esses bloqueadores.

Esta revisão examinou os arquivos reais Rules.h, Save.h, Narrative.h, View.h, controller .ino, menus, bolsa, viagem, poderes e testes. Considera o retorno positivo do usuário, mas não equivale a observar uma sessão na placa. A prioridade aqui é melhorar decisões e reduzir erros evitáveis antes de ampliar conteúdo.

## Dez melhorias propostas

| # | Melhoria | Problema observado e critério de lançamento | Estado nesta entrega |
|---|---|---|---|
| 1 | Inimigos com intenção e padrões de ação | Ataque repetido e inimigos diferentes só em atributos. O jogador precisa antecipar perigo e escolher entre atacar, defender ou curar. | Aplicada: quatro padrões com intenção visível e cadência persistida |
| 2 | Preparação de viagem | O mapa consumia automaticamente ração/mapa/sorte ao tocar Viajar, antes de uma confirmação explícita. | Aplicada: confirmação, nível, CD, chance exata e consumo mostrado; cancelar não altera RNG/save/itens |
| 3 | Comparação antes da compra | Equipe já mostrava comparação, mas compra não mostrava o benefício real em relação ao item vestido. | Aplicada: ATQ/DEF/MP atuais e projetados, sem equipar ou fabricar posse |
| 4 | Recuperação após derrota | Retorno com 1HP oferecia explorar novamente; descanso não repunha poderes se HP/MP estivessem cheios. | Aplicada: preparação com bolsa/acampamento/mapa e correção do descanso |
| 5 | Guia consultável e contextual | Tutorial de uma tela não explica poderes novos, gasto de suprimentos, tipos de dano e ações da dungeon. | Aplicada: sete tópicos, acesso pelo menu/configurações/tutorial/técnicas, volta à origem sem gastar turno |
| 6 | Fechar a campanha de Aeldra | Objective/chapter dizem que atos de Mares/Aurora estão em preparo. Falta levar Anwen, aliados, Odran e Coração do Véu a um clímax e final jogáveis, seguindo o cânone. | Pendente; bloqueador de lançamento completo |
| 7 | Balancear níveis 1–20 e economia | Fórmula de XP oficial não garante tempo de progressão adequado; classes, multiplicação de ataques, preços e loot precisam de simulação e sessões reais. | Pendente: medir tempo por nível, lutas/mortes, utilidade de técnicas e taxa de recompensas |
| 8 | Completar escolhas das classes | Algumas habilidades continuam adaptadas/ausentes: subclasses, preparação de magias, punição divina/auras e recursos avançados. | Pendente: entregar escolhas com custos distintos, não apenas dano maior |
| 9 | Provar atualização e estabilidade na placa | CI e testes nativos não comprovam desligamento durante instalação, Wi-Fi ruim, SD ausente, toque, bateria ou sessões longas. | Pendente: matriz física de interrupção/retomada, recuperação e 5 novos jogadores |
| 10 | Preparar apresentação de lançamento | Faltam créditos consolidados, inventário de origem/licenças das artes, manual de instalação/recuperação, capturas atuais e verificação de legibilidade/acessibilidade. | Pendente; não declarar direitos de assets automaticamente revisados |

Escolhi 1–5 por conseguirem transformar o ciclo atual sem alterar o cânone nem prometer uma campanha completa a partir de conteúdo incompleto. A próxima prioridade é 6–9: fechar a aventura inicial, equilibrar e demonstrar confiabilidade. A meta de diversão é uma sessão curta com objetivo legível, decisão relevante, recompensa compreensível e vontade de continuar por curiosidade e domínio, sem punição por faltar um dia.

## Comparação e critério de direção

[Knights of Pen & Paper 2](https://www.paradoxinteractive.com/games/knights-of-pen-and-paper-2/about) apresenta aventura pixelada por turnos com classes, missões, dungeons e equipamentos. Para POKET, a lição é conectar esses sistemas a uma aventura com identidade, sem tratar cada tela como um produto separado.

[For the King](https://store.steampowered.com/app/527230/For_The_King/) descreve combate por turnos inspirado em rolagens e preparação por acampamentos. Para POKET, preparo e leitura do risco precisam acontecer antes do custo/encounter.

[Legend of Grimrock](https://store.steampowered.com/app/207170/) descreve exploração em grade, combate tático e segredos. Para POKET, a dungeon precisa ensinar como interagir e deixar a vitória legível. Seu combate em tempo real não foi transplantado para o nosso sistema de turnos.

São inferências de design a partir das páginas oficiais, não dados de retenção/vendas nem testes comparativos conduzidos aqui.

## O que muda no jogo

### 1. Intenções e quatro padrões

Novos heróis de progressão D&D usam uma cadência de três ações do inimigo, salva no checkpoint. Lobos, javalis, Guardião, Sentinela, Arconte e hipogrifos atacam, preparam sem dano e então desferem um golpe com dano ×1,5 depois do crítico. A preparação dá uma janela visível para poção/defesa; a guarda dura somente a próxima ação do inimigo. Esqueleto/Vigia se recompõem até4HP na segunda ação. Espectro drena até2MP além do ataque na terceira; esquiva ou guarda75% bloqueiam a perda de MP. Goblin/Saqueador mantêm ataque normal. O risco máximo mostrado inclui o golpe forte, guarda e resistência física da Fúria; preparar/recompor/expulsão mostram risco0HP. O texto de drenagem informa perda de mana mesmo quando o riscoHP é baixo. Não há RNG ao consultar a intenção.

Heróis anteriores à progressão conservam suas fórmulas e os ataques antigos. O Clube da Luta conserva seu protocolo e regras próprios. Não são novas regras oficiais de D&D: os padrões são design original para os encontros do POKET.

### 2. Confirmação de viagem

Selecionar um destino diferente abre Preparar viagem. Mostra nível recomendado, CD, chance segura exata do D20, com natural1 falha/natural20 passa e todos os bônus dos suprimentos atuais. Mostra o consumo de uma ração/mapa/sorte quando disponíveis. Não bloqueia um aventureiro que queira enfrentar um local acima do seu nível. Confirmar consome e rola uma única vez, grava antes da animação; retry de save não rola de novo. Cancelar não gasta, não salva e não avança RNG. D20 e deslocamento no mapa continuam intactos.

### 3. Loja

A compra projeta ATQ, DEF e MPmáximo usando um personagem temporário, sem compra/equipamento real. O item comprado continua indo à bolsa, com aviso para equipar. Comparação considera forja, atributos e item atual; arma de classe inválida não gera benefício fictício. Equipar/retirar e venda mantêm confirmações e regras anteriores.

### 4. Derrota e descanso

Após confirmar uma derrota de exploração/viagem/dungeon/evento, o herói continua com a penalidade já existente e 1HP, mas abre Recuperar forças. Bolsa, acampamento, mapa ou retorno ao local são escolhas do jogador, sem cura ou prêmio automáticos. A derrota continua sendo salva; loot adquirido não é apagado. O descanso do abrigo repõe os recursos de classe mesmo com HP/MP cheios. Vitória/fuga mantêm seus retornos anteriores. O acampamento vitorioso ainda leva ao descanso, sem interromper sua animação.

### 5. Guia

Sete páginas: primeiros passos; combate/intenção; classes; viagem/camp; bolsa/equipamento; dungeon; saves/update. Pode abrir na tela de título por Configurações>Guia, no Menu>Guia, no tutorial>Abrir guia e em Técnicas>Guia. Volta à tela que abriu, sem concluir tutorial nem alterar um herói só por consultar. O combate permanece pausado durante consulta. Imagens anteriores são preservadas fora das novas regiões de interação.

## Saves, arte e limites

Save15 continua128bytes/CRC124 e lê1–15. Byte119 guarda enemyBeat0..2; 120–123 reservados. Os formatos1–14 importam com cadência0, sem recalcular herói, classmode, itens ou recursos. IDs de mapa e enums existentes preservados; TravelConfirm/Recovery/Guide são anexados ao Page. Sem alteração das artes425assets, partições, NVS ou Heltec. Nenhuma placa gravada pelo agente e nenhum .rpg gerado. O firmware anterior não lê save15: não fazer downgrade após salvar sem backup compatível.

As cinco melhorias não fecham Mares/Aurora, não completam D&D nem comprovam estabilidade física. Os itens6–10 continuam trabalhos reais antes de lançamento completo.

## Verificação e testes na placa

22 suítes nativas, com launch_readiness: intenções/rolagens determinísticas/retomada, preparação/cura/drenagem, limite de dano, chance exata do D20, comparação e importação14. Controller real: guia/retorno sem escrita, cancelamento sem custo/RNG, save retry do mesmo dado, descanso com recursos gastos/HPcheio e recuperação após derrota/dungeon. Renderer: sete guias, três confirmações, recuperação,18 quadros de intenções e compra, com limite de texto e pureza de consultas.

Testar na placa: 1. Guerreiro/Bárbaro novos versus Lobo e Esqueleto e contra Espectro; 2. Cancelar/confirmar viagem com suprimentos e manter D20/animação; 3. Comprar arma melhor e conferir comparação/bolsa/equipar; 4. Derrota normal/dungeon e bolsa/acampamento de recuperação; 5. Abrir o guia nas quatro entradas e retornar, sem apagar saves antigos.
