# Pacote4.5 — Conheça o inimigo

Versão2026.10.09-inimigos1/build2026100904. Candidata em validação.

Ficha de combate acessível no seu turno pelo nome/HP do inimigo ou pelo atalho Ficha >. Na dungeon, abra Menu e toque no cabeçalho Ficha do inimigo >. Voltar retorna à origem. A ficha mostra o sprite do adversário atual, HP atual/máximo do encontro, ataque e defesa, tipo físico/mágico, natureza morto-vivo ou criatura, intenção atual e limite superior de dano do próximo golpe com sua defesa e proteção atuais.

Três dicas usam as regras realmente implementadas: preparação sem ataque, golpe forte e defesa, recomposição até4HP ou drenagem até2MP; Fúria resiste apenas dano físico; mortos-vivos podem ser alvos de Expulsar e bônus divino. As dicas sobre poderes não dispensam seus requisitos de classe/nível/juramento/recursos. O limite é uma estimativa máxima, não dano garantido: crítico, defesa e tipo já considerados; esquiva ainda pode zerar o golpe. Não inventa vulnerabilidades/resistências elementais que o motor não tem.

Funciona nos18 inimigos existentes, inclusive mímicos, eventos e chefes encontrados, sem catálogo de chefes futuros. Esta entrega é inspeção do encontro atual; registro persistente de criaturas descobertas/bestiário completo continua pendente. Não existe novo campo nem migração de save, não adiciona XP/preços/dano/rolagens/recompensas. Save19/128bytes/leitura1–19 e459artes SD preservados.

Abrir/navegar/fechar não gasta turno, mana, usos, não avança RNG e não grava save. Bloqueado durante animação/turno inimigo pela mesma lógica dos demais menus de combate; o jogador consulta no seu turno. Reinício retorna ao estado de combate salvo. Reutiliza sprites e fundo atuais em memória; sem leituraSD por quadro.

32suítes incluindo renderer:216 combinações de classe/inimigo/cadência, limites superiores contra dano real, dicas/tipos/read-only, acesso real18inimigos em batalha/dungeonMenu e retorno sem gravações/RNG/mudança de estado. Renderer verifica54fichas com texto/sprites. Testes de classes/progressão/loja/exploração/trancas/dungeon/save/viagem preservados. Snapshots do controlador real conferidos.

Teste físico: atualizar em Configurações > Atualização; na batalha, toque Ficha > abaixo deHP. Confira dados/dica, volte e ataque normalmente. Durante preparo/golpe forte/recomposição observe as mudanças de intenção; numa dungeon, Menu > Ficha do inimigo > Voltar. Nenhuma consulta deve gastar turno ou mudar recursos. Teste físico fica com o usuário. Sem flash automático/.rpg/eraseNVS/formataçãoSD/alteraçãoHeltec.

Próximos: bestiário com memória de descobertas, balanceamento real/economia, lacunas das classes; Pacote5 Aurora e finais continua planejado. Não chamar esta ficha de registro persistente já entregue.
