# Pacote 4.2 — Guerreiro: Trilha do Campeão

Versão2026.10.09-campeao1, build2026100901. Publicada e verificada:29suítes e compilaçãoESP32-S3 passaram; downloads conferidos por tamanho/SHA-256/CRC. Teste físico pendente do usuário.

Guerreiros com progressão D&D agora recebem três marcos passivos:

- Nível3: Campeão / crítico aprimorado. Chance20% em Ataque e técnica com arma.
- Nível15: crítico superior. Chance30% nos mesmos golpes.
- Nível18: Sobrevivente. No início de cada turno de combate, se estiver vivo e com metade do HP ou menos, recupera5 + modificador de Constituição, limitado ao HP máximo. O limiar é verificado antes de curar; a cura pode ultrapassar metade da vida.

Referência2014/SRD5.1: https://media.wizards.com/2023/downloads/dnd/SRD_CC_v5.1.pdf. Os níveis3/15/18 e a regra de recuperação seguem o Campeão. Adaptação explícita: o motor do RPG POKET usa uma chance percentual de crítico de12% e dano crítico existente(+50%); não usa a rolagem D20 de ataque de mesa. Por isso as faixas19–20/18–20 foram adaptadas para20%/30%, conservando uma melhora real sobre12%. Não são as probabilidades de D&D (10%/15%). Uma ação de Ataque adicional continua agregando golpes com uma rolagem de acerto/crítico. A chance inimiga continua12%, sem receber o benefício do Guerreiro. Mago/Paladino/Bárbaro e heróis legados conservam suas chances.

Esta entrega aplica os benefícios do Campeão à linha atual do Guerreiro; não grava escolha permanente de subclasse e não anuncia outros arquétipos como implementados. Atleta notável, estilo adicional e Indomável continuam pendentes. Sem poderes ativos novos, mana ou consumíveis para essas passivas.

Poderes do Guerreiro possui quatro fichas: Segundo fôlego, Surto, Trilha do Campeão e Sobrevivente. As passivas mostram chance, cura, limiar e nível. Tocar a ficha/status não lança um poder, não cura, não gasta recursos, não avança turno e não grava save. Trilha de classe/próximo marco mostram somente ganhos executáveis.

Sobrevivente é aplicado uma vez na entrada do combate e uma vez após a ação inimiga, inclusive preparação/cura do inimigo. Não se aplica em navegação, menus, descanso, caminhada na dungeon ou ao restaurar save. Surto concede outra ação dentro do mesmo turno e não repete a cura. HP0 continua derrota; não ressuscita. O HP recuperado faz parte da transação existente de combate, portanto reiniciar/repetir gravação não repete o benefício.

29suítes:30.000 ataques comparados nas três chances, preservação do inimigo/legados/outras classes, limiar de cura/nível/HP0, preparação/cura inimiga, Surto, save19 e fichas passivas sem gravação. Renderer verifica texto/estado e níveis1/3/15/18/20. Benchmark40.960 combates emcampeao-balance.csv: recursos cheios por combate, sem equipamento/poções/forja/atributos comprados, não é sessão real ou alvo global de dificuldade. XP/preços não mudam.

Save19/128bytes/leitura1–19 permanece igual; sem novo campo/migração. Artes459 entradas idênticas àpaladino1; efeitos existentes em RAM, sem leitura SD por quadro. Preservados mapa/D20/deslocamento, dungeon, missões, trancas e Paladino.

Teste físico: Configurações > Atualização. No Guerreiro, Técnicas > Poderes > avançar até Trilha do Campeão/Sobrevivente. Nv3/15: conferir chance exibida e golpes críticos ao jogar; não é garantido a cada quantidade fixa de ataques. Nv18: com HP baixo, entrar em combate e observar recuperação após turno inimigo; Surto não cura de novo. Tocar ficha passiva não muda HP/MP/turno. Retomar save mantém HP. Legados seguem indicação de regras preservadas. Sem flash automático, .rpg, eraseNVS ou formataçãoSD.

Próximos desdobramentos4: Mago/Bárbaro, outros recursos/bestiário e medição real de balanceamento. Pacote5 Aurora segue planejado.
