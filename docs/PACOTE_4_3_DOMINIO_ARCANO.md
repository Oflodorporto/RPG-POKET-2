# Pacote 4.3 — Mago: Domínio arcano

Versão2026.10.09-arcano1, build2026100902. Publicada e verificada: 30 suítes e compilação ESP32-S3 passaram; downloads conferidos por tamanho/SHA-256/CRC. Teste físico pendente do usuário.

Dois marcos funcionais para Magos com progressão D&D:

- Nível10, Evocação aprimorada: soma o modificador de Inteligência (mínimo0) uma vez ao dano total de Mísseis mágicos, Mãos flamejantes, Raios abrasadores e Bola de fogo. Não soma por dardo/raio. Se todos os raios errarem, não há dano extra. Nas magias com resistência, o bônus entra antes de reduzir o dano à metade. Não afeta o ataque básico de cajado nem Escudo arcano.
- Nível18, Maestria em magia: Mísseis mágicos e Raios abrasadores custam0MP. Conservam dados/dano/testes/alcance/turno e continuam sendo conjurações normais. Escudo/Mãos/Bola de fogo conservam seus custos. A técnica de ataque do Mago, que chama Mísseis, também mostra0MP e funciona com mana0.

Referência2014/SRD5.1: https://media.wizards.com/2023/downloads/dnd/SRD_CC_v5.1.pdf. Evocação aprimorada10 soma INT a uma rolagem de dano; o motor agrega a magia num alvo, portanto soma uma vez por conjuração. Maestria18 usa uma magia de1ºcírculo (Mísseis) e outra de2º (Raios abrasadores). Adaptação: par fixo desta entrega em lugar da escolha livre de magias de mesa, mana substitui slots, alvo único existente. Nenhuma amplificação de círculo/AOE/tradição permanente é introduzida. Recuperação arcana, outros recursos de tradição e Magias de assinatura continuam pendentes; não aparecem como ganhos executáveis.

Grimório passa a ter sete fichas: cinco magias existentes e as duas passivas. Cada ficha informa nível/efeito/desbloqueio; passivas não executam ações, gastam recursos ou gravam saves ao tocar. Custos exibidos nos atalhos de Técnicas e no Grimório usam a mesma regra real. Trilha/próximo marco incluem10/18. Magias continuam com animações existentes em memória, sem leitura SD por quadro.

30suítes:4.000 comparações com/sem Evocação, bônus único, raios que erram, redução por resistência, INT, limites/legados/classes, Maestria17/18, mana0, turno/recompensa/save19 sem repetição. Controlador real testa fichas passivas sem gravação e falha/repetição de save após magia gratuita sem repetir ataque. Renderer verifica texto/técnicas/mana0 e níveis9/10/17/18/20.

Benchmark40.960 combates emarcano-balance.csv, com recursos cheios por combate, sem equipamento/poções/forja/pontos. Política de poderes tenta Bola de fogo quando tem mana; no18 usa Raios grátis quando ela acaba. Não equivale à sessão de campanha nem alvo global de dificuldade. XP/preços permanecem iguais.

Save19/128bytes/leitura1–19 permanece byte a byte compatível; sem campo/migração nova. Legados permanecem nas regras antigas. As459artes SD são idênticas àcampeao1. Paladino/Guerreiro/Bárbaro, mapa/D20/deslocamento, dungeon, exploração/trancas e campanha preservados.

Teste físico: Configurações > Atualização. No Mago, Técnicas > Poderes (Grimório) > avançar até Evocação/Maestria e conferir bloqueio/nível. Nv10: dano das quatro magias recebe o bônus; cajado/escudo não. Nv18: Mísseis/Raios0MP, inclusive com mana0; cada magia gasta turno, outras ainda exigem mana. Tocar passivas não muda HP/MP/turno. Retomar após conjuração mantém o resultado. Teste físico fica com o usuário; sem flash automático/.rpg/eraseNVS/formataçãoSD.

Próximo desdobramento4: Bárbaro e recursos pendentes/bestiário/balanceamento real. Pacote5 Aurora continua planejado.
