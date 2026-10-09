# Pacote 4.4 — Bárbaro: Fúria ancestral

Versão 2026.10.09-barbaro1, build2026100903. Candidata em validação.

Crítico brutal agora usa dados reais: +1d6 no nível9, +2d6 no13 e +3d6 no17, apenas quando o ataque ou a técnica com arma acerta e é crítico. Substitui o bônus fixo+1/+2/+3 antigo. O bônus entra uma vez por ação, depois do multiplicador da técnica: não é duplicado por Ataque extra nem pela técnica. Não altera a chance de crítico12%. Não gasta mana nem rola dados extras em golpes esquivados. Fúria/Armas conservam seus demais bônus.

Nível15, Fúria persistente: uma Fúria ativada deixa de expirar após três rodadas e dura até terminar o combate. Vitória/derrota/fuga limpa o efeito; entrar em outro combate exige ativar de novo. Não restaura usos; no20 continua sem limite. Descanso completo continua restaurando os recursos. Ações de cura, preparação inimiga, ataque mágico e retomada do save não encerram a Fúria persistente. Antes15 mantém três rodadas.

Poderes do Bárbaro passam a ter três fichas: Fúria e duas passivas, com níveis/estado/explicação. Tocar uma passiva não executa Fúria, gasta recurso ou grava save. A ficha ativa e a mensagem de ativação mostram a duração correta. Trilha/próximo marco anunciam ganhos executáveis9/13/15/17.

Referência oficial2014: https://www.dndbeyond.com/sources/dnd/basic-rules-2014/classes#Barbarian. Crítico brutal9/13/17 acrescenta1/2/3 dados de arma; o motor atual não possui dado por equipamento, portanto esta entrega usa d6 fixo e uma rolagem agregada por ação. Fúria persistente15 é adaptada ao encontro, encerrando também ao fugir/terminar a batalha. Não é implementação integral do Bárbaro de mesa. Fúria implacável11, Força indomável18, aumento de atributos de Campeão primitivo20, Ataque imprudente e Caminhos permanecem pendentes. Força indomável exige atenção à validação de baús já resolvidos: não invalidar saves19 antigos mudando retroativamente seu resultado.

31suítes mais render:14.000 ataques do Bárbaro, limites de dados, níveis/classes/legados, esquiva sem dados extras, persistência14/15/20, resistências/cadência, término/retomada, recompensas únicas e saves19. Testes do controlador real verificam fichas passivas sem gravação; prévias e limites de texto9/13/15/17 e adjacentes. Comparação controlada40.960combates embarbaro-balance.csv: recursos cheios por combate, sem equipamento/poções/forja/pontos e inclui regiões inadequadas; não é sessão real ou alvo global de dificuldade. XP/preços permanecem iguais.

Save19/128bytes/leitura1–19: nenhuma alteração de formato/campo/migração. Fúria ativa de um save antigo no15+ preserva seu valor e passa a persistir. Demais classes e legados preservados.459artes iguais àarcano1; sem leituraSD por quadro. Sem flash automático/.rpg/eraseNVS/formataçãoSD/alteraçãoHeltec.

Teste físico pelo usuário: Configurações > Atualização. Bárbaro > Poderes: confira fichas e estados antes/depois9/13/15/17. No15, ative Fúria e jogue mais de três rodadas, incluindo cura/preparação; confirme que permanece e termina ao vencer/perder/fugir. Retome o jogo durante a luta e confira usos/duração. Antes15 deve terminar em três rodadas. Ataques críticos recebem dado(s) extra; demais ataques não. Mago/Paladino/Guerreiro conservam seus poderes. Testes/compilação não confirmam placa física.

Próxima etapa: priorizar recursos pendentes com compatibilidade e balanceamento de sessões reais; Pacote5 Aurora/finais permanece planejado.
