# Pacote 10: qualificação do duelo PvP

## Estado e limites de autorização

Este é um candidato de qualificação em branch separada. Preparar o pacote e as duas políticas de resultado foi autorizado em 14/10/2026; main, tag, release, gravação USB e OTA física de PvP não foram autorizados. A infraestrutura de publicação fica inativa nesta branch e exige futuramente um push em main do mesmo SHA já qualificado, além da autorização específica para publicar.

A preparação inicia sobre clock77. O parent só pode ser congelado depois da verificação dos bytes públicos de clock77 e de seu commit final de README. Os pins exatos constam de `tests/fixtures/package10-published-baselines.json`; não usar bytes da qualificação clock como se fossem os bytes recompilados pelo publisher.

Identidade proposta: `2026.10.14-pvp77`, build `2026101404`, pacote `7.7 - Duelo PvP`. Não misturar com os intermediários locais v5/48 bytes: o candidato final usa protocolo v5/52 bytes. O predecessor público usa v4/44 bytes. A UI bloqueia convite incompatível e informa **Atualize as duas placas**. Não existe migração do protocolo durante uma partida.

## Conteúdo preservado e mudança permitida

O patch combinado de PvP, SHA-256 `ac9e3725b42cf1b0de949de8cac770673e0f8bf15a88c342dfb7fde0e1675b16`, contém 27 arquivos vinculados ao manifesto de fontes SHA-256 `caf2bd0c11924ee848d4618526867b4b62d6dab370ebed6ae687edce091764f9`. A primeira qualificação revelou uma dependência de link no renderer. O ajuste mínimo abaixo altera apenas dois arquivos desse conjunto, sem mudar os corpos das consultas ou as regras; os demais 25 arquivos continuam idênticos.

Apenas a identidade em ReleaseVersion.h é exceção de packaging. ClockCycle7.h/ClockView.h, narrativa, Rules, Controller, persistência, Save30, epoch1, combate PvE, RNG, economia e prêmios de campanha continuam os do predecessor. Bolsa visual nova, pets e coop não fazem parte deste pacote.

- Aparência local/remota com classe, raça e cores admitidas na sessão.
- HP/MP, guarda, custos de mana e feedback de ação confirmada, usando arte já existente.
- Desistência fora do turno e prioridade correta para prova terminal em relação a Cancel.
- Resultado sem prova após o início: **NAO CONFIRMADO**, sem prêmio, entrada já consumida. Antes do início permanece a devolução integral existente.
- Desistências simultâneas: prevalece a solicitação do dono do turno acordado. O outro pode ganhar mesmo tendo solicitado desistência.
- Prêmio normal permanece 45 após a entrada de 25, saldo líquido +20; derrota consome 25; empate devolve 25. Nenhuma fórmula PvE é modificada.


### Correção de integração após o primeiro CI

O run 38575517453, SHA 9841c466d1755f803246727a8a9cabf5c10d8483, passou os 13 novos alvos PvP, mas falhou em 32 alvos herdados de render/controller. A reprodução mostrou referências externas a Duel::canPlay e Duel::cost introduzidas pela tela, enquanto esses harnesses reais foram desenhados para compilar somente os headers.

A correção move os corpos inalterados de price/cost/legal/canPlay de ClubDuel.cpp para definições inline em ClubDuel.h. Não adiciona stubs, cópias de regras, mudanças de preço ou novo estado. Nenhum harness legado foi alterado. Um teste novo de configuração executa 55.296 combinações de estado, classe, mana e ação, em O0 e O2, ligando apenas o header real. O plano continua com as mesmas 143 tarefas, e todo o agregado será reexecutado no novo SHA. O runner também mostra o final do log de qualquer alvo que falhar, sem esconder a falha.

Patch mínimo SHA-256: 86d2711a32e25a4b098521e9a272e088553c3390192b168c80f78184b8c17671. O inventário congelado vincula explicitamente os dois hashes posteriores à correção. A evidência do CI anterior não qualifica o novo SHA. A alteração não permite ignorar o gate ESP32/OTA nem os testes físicos pendentes.

## Persistência e confiança

`tests/pvp_reliability_save30.cpp` usa ClubRadio, PersistentSlotBackend e PersistentJournal de produção com Save30/epoch1 e store host de blobs com falhas. Testa falha antes/depois da escrita, retorno falso confirmado por readback, retry, reboot e dez fronteiras simuladas de liquidação: sete recuperam perda conservadora e três a vitória durável. Crédito durável no máximo uma vez não significa conclusão atômica ou exatamente uma vez. Uma vitória confirmada somente em RAM pode ser perdida se a energia cair antes do commit.

Os 48 cortes da suíte herdada `fatigue76_club_radio` usam FatigueJournal/Save28 legado. Não são cobertura do journal atual. A suíte Save30 permanece separada e obrigatória no CI. Os fakes de rádio/FreeRTOS/store não simulam física de flash, RF ou driver ESP-IDF.

MAC, perfil, sequência e digest são filtros lógicos, sem autenticação criptográfica de aplicação ou promessa antifraude. Não há consenso durável, recuperação de resultado por reconexão nem garantia contra perda arbitrária infinita.

## Qualificação exigida

1. Inventário completo e hashes congelados, sem inputs extras, symlinks, saída compilada dentro do sketch ou binários de release rastreados. Arte reconstruída é a mesma de 666 assets, 7.470.928 bytes totais, SHA-256 `14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56`.
2. Plano de 143 tarefas: as 130 herdadas sem substituição, mais 13 alvos PvP. Todos os 13 novos alvos rodam em otimizado e ASan/UBSan; LeakSanitizer é obrigatório no caminho CI. Os scripts locais antigos documentam `detect_leaks=0` para o ambiente ptrace e não são chamados pelo agregado.
3. As reproduções independentes estão em `tests/pvp_review_integration.cpp`, `pvp_review_ordering.cpp`, `pvp_review_render.cpp` e `pvp_review_equivalence.cpp`. O último gera 245.760 transições pareadas e `tools/check_pvp_equivalence.py` exige o digest determinístico do trace já comparado ao reliability-v2. O fuzz de 100 mil pacotes exercita decoder/memória, não 100 mil partidas nem cobertura exaustiva.
4. Renderer real: 15.030 renders por modo, 2.048 identidades, 5.376 fases, 596 capturas idênticas entre modos e 12 GIFs. Capturas são pixels de código real; 60 ms é amostragem nominal, não FPS físico medido.
5. ESP32-S3/core 3.3.12, PSRAM OPI e partição app3M_fat9M_16MB; compilação isolada com `--export-binaries=false`, verificando o SHA e o inventário novamente após o build. Packet possui static_assert de 52 bytes na produção compilada; os fakes host não são inputs do sketch real.
6. Manifesto/firmware/arte reais do próprio build passam verificador, testes negativos e adapters OTA otimizados/sanitizados. touch77 recusa; scenes77/scale77/camp77 migram Save29 de forma lazy; explore77/immersion77/story77/clock77 preservam Save30. Cada fonte Save30 cobre 15 registros nos três slots, casa e ambos os formatos de andares. Read-back usa os codecs históricos exatos, sem alegação de downgrade do firmware.
7. Identidade final vincula SHA/tree, plano, fontes, logs, compilador, artefatos, histórico público e capturas. O candidato só é aprovado depois da conclusão de todos esses gates no SHA exato.

A suíte host completa, build real e OTA real ainda não são declarados aprovados por este documento. A evidência de sucesso, quando existir, deve ser lida no run desse SHA e em `package10-source-identity.json`.

## Memória e hardware

Não há novo payload de arte, framebuffer, atlas ou chamada de alocação de produção. O orçamento conhecido PSRAM permanece 7.638.272 bytes. No host, ClubUi cresce de 560 para 640, Session 440 para 488, Duel 88 para 104, Packet 44 para 52 e Peer 40 para 44; a fila de oito entradas cresce 64 bytes. ClubVisual isolado tem 16 bytes, mas não representa todo o aumento de RAM. Temporários do renderer têm ViewState=656 e Game=184 bytes no host.

Nada disso mede stack high-water mark, heap livre, PSRAM real, FPS, toque, latência ou interferência RF. O firmware deve caber nos 3 MiB de aplicação, verificado nos bytes finais. Teste de duas placas, iluminação/legibilidade física, SD, Wi-Fi/ESP-NOW e cortes reais de energia continuam pendentes. Não transformar atualização story/clock realizada pelo usuário em aprovação física global de PvP.
