# Cripta do Arconte — dungeon1

240×320: visão em primeira pessoa de 240×172, barras de estado e seis controles grandes embaixo. Movimento por casas: avançar/recuar, girar 90 graus e andar de lado. Toque curto na visão ou no botão AÇÃO interage; esta primeira entrega usa botões, sem gestos de deslizar.

Entrada nas Ruínas Antigas, botão Dungeon / Nv 8+. Consome um cristal por expedição. O Guardião das Ruínas deixa um cristal por vitória, até o limite de nove. Loja > Suprimentos > Cristal: 300 ouro. Nível 8 é recomendação, não bloqueio. Leve poções.

Dois labirintos de 9×9: cripta cinza e santuário azul. Há seis encontros regulares e o chefe Arconte. Combate por turnos na própria visão; AÇÃO ataca, Menu oferece técnica, defesa, poções e fuga. Entrar na casa de um inimigo inicia combate sem sobrepor o personagem ao inimigo. Para vencer é preciso confirmar CONTINUAR depois do golpe final.

AÇÃO recolhe ouro/poções na casa atual e usa escadas. O selo do primeiro andar abre a porta do segundo. Baú da sala final exige derrotar o Arconte; dá arma de segunda categoria da classe, sem equipar automaticamente. Se já possuir essa arma, recebe 50 ouro. Os outros montes dão 25 e 60 ouro.

Sair pede confirmação, mantém o saque e encerra a expedição; a próxima entrada reinicia o labirinto e custa outro cristal. Derrota mantém o comportamento de penalidade do jogo e retorna às Ruínas com 1 HP. Fuga não mata o inimigo. Cada movimento, giro, coleta e turno usa o journal com verificação; falha de save pausa para tentar novamente. Reiniciar retoma posição, direção, andar, loot e combate.

## Assets originais

Gerados com a ferramenta integrada imagegen. Prompts completos: assets/dungeon1/PROMPTS.json. Cinco atlas originais preservados; ferramentas de conversão: tools/prepare_dungeon_art.py. Não houve modificação artística por código: apenas recorte das células, redução nearest-neighbor e conversão RGB565.

44 assets: 16 poses dos quatro inimigos (esqueleto, espectro, vigia e Arconte), 12 objetos (moedas pequenas/grandes, baú fechado/aberto, cristal, selo, vida, mana, tocha, escadas para cima/baixo e relíquia), oito texturas dos dois andares e oito poses de arma. Mago usa cajado, cavaleiro espada/escudo, guerreiro lança conforme o equipamento já existente, bárbaro machado. Repouso/ataque para cada arma.

192512 bytes RGB565 na flash. Renderização por raycasting de 120 raios e pixels 2×2 no framebuffer existente, com profundidade para ocultar inimigos atrás das paredes. Sem leituras de SD durante a dungeon. O pacote artes.pak estradas1 continua válido; esta arte acompanha o firmware. Tocha, baú aberto e relíquia também estão preparados para expansão; não são todos colocados no primeiro labirinto.

## Saves e validação

Save9 mantém 96 bytes e importa saves1..8. Usa bytes antes reservados, sem mudar namespaces ou partições. Não instala versões antigas por cima de save9: elas não sabem ler o novo formato. Firmware manual por cabo; nenhuma gravação automática, formatação ou pacote .rpg.

wifi1: usuário confirmou que todos os testes físicos deram certo antes desta etapa. dungeon1 ainda precisa de validação na placa: alcance/legibilidade dos controles, fluidez do raycasting, retomada após reinício, chefão e tesouros.
