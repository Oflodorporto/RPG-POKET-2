# Pacote5 adicional — A Ilha Revelada

Versão2026.10.09-ilha1. Candidata em validação; não gravada automaticamente.

A Ilha das Marés não aparece nem é mencionada durante a campanha. Somente final completo (campaignEnding=2, quatro contribuições e reconstrução) revela o novo ponto no mar. Final temporário não libera. Entrar pelo novo ponto do mapa; entrada livre, recomendada para nível20. O mundo e destinos0..3 não mudam.

Três andares com texturas distintas, duas alavancas nas paredes (procure no corredor oeste de cada um dos dois primeiros andares), três armadilhas no segundo andar e Thalvor na sala final. Inimigos: Sentinela Coral, Arraia Runar, Corsário Afogado, Oráculo Abissal e Thalvor. Cada batalha concede XP/ouro uma vez, agora mostrado também nas vitórias comuns da primeira dungeon. Ao nível20 o limite de progressão continua valendo.

Armadilhas testam D20+Destreza vs14 e não matam sozinhas. Ao sair ou perder, o labirinto é reiniciado; os itens já coletados permanecem na bolsa. O chefe abre o baú com equipamento de outra classe ou600ouro. A vitória fica registrada no personagem.

Save24 mantém128bytes e lê1..24. Byte67: guildMember bit0/descobertas20..24 bits1..5. Byte119:enemyBeat bits0..1/armadilhas bits2..4/vitória permanente bit5. DungeonFlags:bit4 tipoilha/bit5 andar3. Bytes90/91 acampamento,122/123 origem,124CRC preservados. Sem downgrade para firmware que só lê23 após gravar24.

Artes480: carga6910016bytes; somente63488bytes adicionados após os459 anteriores. PSRAM carrega no início; sem leituras de cartão a cada quadro. Sprites originais gerados com imagegen integrado, originais e prévia em assets/island1. Sem.rpg, flash automático, formataçãoSD, NVS erase ou alteração Heltec.

Teste físico: atualizar firmware e imagens; terminar os quatro faróis normalmente. Confirmar ausência da ilha antes do final, entrada após o pleno, alavancas/toque na cena, subida/descida, armadilhas, XP, reinício no meio da dungeon, vitória/baú/saída ao mapa. Os testes de software não substituem essa etapa.
