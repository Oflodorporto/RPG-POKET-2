# Pacote 6: contrato de qualificação local

Candidato inédito `2026.10.13-explore77`, build `2026101309`, pacote `7.7 - Dados e exploracao das Ruinas`. Não publicado. Migração Save30 autorizada; publicação, tag, promoção de main, instalação e envio remoto exigem autorização separada. Este documento define gates, não declara que CI/ESP32/placa passaram.

## Fonte congelada e escopo

Base qualificada Pacote5: commit `7a05617cd1e13843d73de72f970c57a7113af48a`, árvore `b58a5c80339f1d87728723991efdc8006ff764f8`. Integração funcional congelada: árvore `814f16a9f6e11a2d5fe0cf6a32f7da67c92bbb17`. Esta preparação usa cópia isolada dessa árvore. O inventário específico `tests/fixtures/package6-approved-sources.json` fixa todos os arquivos funcionais e os arquivos herdados permanecem byte a byte, salvo exceções explícitas de empacotamento. Os gates, testes, workflows de qualificação e inventários históricos de Pacote5 e anteriores não são reescritos para aceitar Save30. O workflow único de publicação é atualizado explicitamente para o novo gate.

Inclui os dados e recibos reais de combate, janela Dados, pulo da apresentação e quarentena de toque/temporizador; captura não avança RNG nem altera combate. A faixa animada mostra os dados explícitos de dano/cura. Variação de arma, crítico e esquiva constam fielmente dos recibos, sem prometer todos esses campos na faixa animada.

Inclui geração R5 nas duas plantas das Ruínas para novas entradas pagas, com diversidade inicial de grafo/encontros. Preserva custo, inimigos/recompensas, pré-requisitos de acesso à cura e faixa de desafio documentada. Mapas da Ilha e de expedições de cartas continuam fixos. D20, tenda, personagem no acampamento, Bolsa nas Ruínas e arte666 do Pacote5 permanecem.

O baseline procedural sem trace está incluído em 12 headers rastreados e congelados por SHA-256, permitindo o diferencial Save30 em um clone limpo, sem depender de outro diretório de trabalho. Os sete fontes do review independente de persistência também são rastreados; os headers históricos são extraídos do commit scenes77 exato.

## Save30, sem novo reset

- Manifesto honesto: writes30, reads[29,30], epoch1. `reset_game_slots_once=[1,2,3]` continua sendo a declaração da época1 histórica, não uma nova solicitação de reset. Marcador Done impede repetição.
- Save29 é lido sem escrita; cada slot migra para Save30 apenas no próximo save normal. Não há sweep nos três slots.
- Runs Save29 ativos mantêm geometria fixa, piso, posição, direção, máscaras, recursos e RNG. Escadas ou um save não convertem a tentativa atual. A próxima entrada paga nas Ruínas usa R5.
- O journal continua em `pkt2_v29`, 160 bytes, CRC156, epoch144, slot152. Só os bytes reservados passam a guardar metadados sob formato30.
- Firmware Save29 antigo recusa slots já migrados, inclusive journal misto29/30. Não joga a cópia antiga, não apaga e não reseta. É necessário firmware compatível Save30 para reabrir esses slots. Esse impacto foi divulgado antes da autorização da migração; publicação segue separada.
- touch77/Save28 continua exigindo instalação USB com `EraseFlash=none` e a fronteira/reset histórico da época1; não há promessa de importar saves1–28 nem de OTA direto.

O gate OTA compila as closures originais de touch77, scenes77, scale77 e camp77/Pacote5 nos commits fixados. Os três clientes Save29 devem aceitar o candidato exato. Testa as três slots em cada caminho, migração real para30, geometria legada, epochDone sem escrita, Wi-Fi/artes, rollback com leitor29 real e bytes30 reais, reparo de arte, hashes, corrupção, metadados, downgrade e falhas de gravação. Fixture sintética é explicitamente host-only; nunca substitui a compilação ESP32 e os mesmos testes com o binário real.

## CI completo e recursos

`package6-check.yml`: branch exclusiva `feat/package6-release`, contents:read, fetch-depth0, LSan habilitado (`RPG_DETECT_LEAKS=1`). Executa `test_package6.sh` sem `--focused`: agregado herdado completo 91 otimizado/50 ASan-UBSan, suites procedurais/trace, diferencial em quatro modos, cinco suites Pacote5 e duas combinadas. Executa ainda review de 9.216 fixtures29 e falhas journal/NVS, e OTA/rollback com clientes reais. As suites focadas repetem regressões relevantes; não se somam resultados duplicados como testes distintos.

Placa Waveshare-29667, ESP32-S3/240MHz, flash16MiB, OPI PSRAM8MiB, core3.3.12, partição `app3M_fat9M_16MB`, `EraseFlash=none`. Binário deve conter identidade exata e caber em3.145.728 bytes. CI registra tamanho, margem, SHA256, commit e árvore exatos. Arte666 permanece total7.470.928/payload7.470.912, CRC `9b4bcfc0`, SHA256 `14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56`.

PSRAM conhecida:7.638.272 bytes (arte+framebuffer153.600+workspace13.760). Gerador usa cache estático198 bytes no ABI host; recibos são RAM-only, Game184 e save160 no host. Essas contas não medem heap livre, stack, FPS ou latência da placa. Capturas de trace, Ruínas, integração, D20/tenda/acampamento são retidas para revisão; capturas não são aprovação visual nem física.

## Gate de publicação separado

O workflow único `release.yml` passa a tratar explore77, mantendo os verificadores/testes históricos intactos. Só executa em main e exige sucesso de `package6-check.yml` no mesmo repositório, branch e SHA exatos. Uma branch de qualificação nunca publica, nem por despacho manual. Testes negativos verificam branch, tag, SHA, origem, workflow e status errados. O trigger main-push mantém o fluxo existente de promoção por fast-forward depois da aprovação; não requer nova credencial, variável, secret nem autorização persistente.

Somente depois de aprovação específica de publicação seria permitido promover exatamente o SHA qualificado para executar o gate. Nova edição ou merge exige nova qualificação. O gate reroda testes, build e validação, recusa tag existente, fixa tag no SHA e confere os três downloads públicos e manifesto latest. Preparar este workflow não concede permissão de executá-lo.

## Verificação ainda separada

O relatório local da preparação acompanha hashes e resultados realmente observados. Compilação ESP32 e LSan dependem do ambiente apropriado; execução ASan/UBSan com `RPG_DETECT_LEAKS=0` não conta como LSan. CI remoto, downloads publicados, instalação, toque, heap/FPS e energia na placa só podem ser declarados após evidência própria. Nenhuma dessas etapas é inferida de testes host.
