# Bolsa, ataques e vitória — 2026.10.06-bolsa1

O usuário confirmou que dungeon1c funcionou na placa. Esta etapa melhora a apresentação e os controles; o formato dos saves continua 9, sem migração, alteração de partições ou troca do pacote de artes do cartão.

## Bolsa

Na dungeon, o botão **Bolsa** ocupa o lugar de AÇÃO e tem uma área de toque maior. O toque na cena continua sendo ataque/interação. O menu da dungeon tem Técnica, Defesa, Bolsa, Fuga e Saída; poções ficam na bolsa.

Seis slots de consumíveis com imagem e quantidade: Vida, Mana, Cristal, Ração, Mapa e Sorte. Ouro aparece no cabeçalho. Selecione o slot e toque em Usar para beber uma poção; em combate isso consome o turno pelas regras existentes. Fora de combate recupera HP/MP e salva. Cristais não são consumidos ao abrir a bolsa ou ao tocar em Usar: são chaves gastas apenas ao confirmar a entrada da cripta nas Ruínas. Suprimentos são usados automaticamente na viagem conforme as regras existentes.

**Equipamentos** abre outra grade de seis slots por página, com ícone, quantidade 1, nome do item selecionado e estado Equipado/Guardado. Cada equipamento é único no catálogo atual; não existem pilhas de armas. Há três páginas se os 18 itens estiverem guardados. Pode equipar dentro da dungeon fora de combate; classe e nível mínimo continuam valendo. A troca não recupera mana.

O baú entrega uma arma de nível de equipamento 2 para a classe: Cajado do Trovão, Espada da Guarda, Lança do Vento ou Machado Feroz. Não é um item separado chamado “Relíquia”. A tela **BAÚ ABERTO** identifica a arma e oferece **Ver na bolsa**, abrindo seu slot em Equipamentos. Se já possuía a arma, recebe 50 ouro e a mensagem explica a duplicata. Não equipa automaticamente.

## Vitória e retorno

Depois de derrotar o Arconte e terminar o efeito, a tela **ARCONTE DERROTADO!** mostra XP/ouro, explica que o baú está liberado e oferece **Explorar** ou **Sair**. Explorar preserva a expedição para pegar o baú e o restante do saque. Sair preserva os ganhos, encerra a expedição e volta às Ruínas; uma nova entrada exige outro cristal. Reiniciar antes de escolher retoma a tela de vitória a partir do turno salvo. Resolver/fechar a expedição não repete as recompensas.

Vitória, fuga ou derrota de encontros normais retorna à exploração da cidade atual (Ruínas para o Guardião), mantendo o fluxo de viagem quando o combate é uma emboscada. O jogador não é enviado automaticamente ao refúgio.

## Animações

Mago: projétil mágico com trajeto e expansão no impacto; raio da técnica mais amplo, com variação e clarão. Guerreiro: estocada com avanço da lança. Cavaleiro e bárbaro: corte; Fúria tem rastro largo e impacto próprios. O cajado/espada/lança/machado tem preparação, avanço e recuperação; a pose de dano do inimigo aparece no impacto. Espectro utiliza efeito de projétil na sua vez. Defesa conserva a barreira azul, distinta de dano.

Esta etapa muda **apresentação**, não o alcance das regras: a dungeon continua com encontros adjacentes. O plano de alcance de três casas/cajado, duas/lança e uma/corte está em COMBATE_DUNGEON_PROXIMA_ETAPA.md e exige persistência de alvo/distância para retomada segura. Não apresentar os projéteis visuais desta versão como combate livre à distância já implementado.

## Validação

O harness dungeon_controller.cpp utiliza o controlador real do sketch e testa acesso local, poção, cristal preservado, equipamento, falha de salvamento e nova tentativa, retomada dos itens, opções do chefe, baú e retorno do Guardião. O renderizador testa limites de texto e todas as páginas, slots, quantidades máximas, páginas de equipamento e fases dos efeitos. As regras de saves, economia e dungeon permanecem nas demais suites.

Não houve gravação automática, escrita ao cartão, apagamento de NVS ou geração de .rpg. O usuário valida o comportamento real após instalar pelo Wi-Fi ou pelo cabo.
