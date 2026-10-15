# Estado publicado · 15 de outubro de 2026

[Apresentação](../README.md) · [Instalação e controles](GUIA_RAPIDO.md) · [Histórico completo](historico/README-2026-10-14.md)

## Firmware atual · correção do treino cooperativo

**Firmware atual:** 7.7 — Treino cooperativo: toque, saída e convite, `2026.10.15-coopfix77` / build `2026101501`, tag `v2026.10.15-coopfix77`. [Release](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.15-coopfix77) · [Uso e limites](RELEASE_COOPFIX77_NOTAS.txt) · [Qualificação da correção](COOPFIX77_RELEASE_QUALIFICATION.md) · [Controles](COOP_TRAINING_CONTROLS.md).

Qualificação de software, publicação e downloads públicos verificados no SHA exato, com auditoria independente concluída. **O reteste físico desta correção permanece pendente.** Os registros completos de coop77 e shop77 estão preservados abaixo como histórico.

### Correção e limites

- **Toque:** coalescência de amostras consecutivas idênticas, preservando bordas, mudanças de coordenada/estado/erro e tempos inicial/final. O limite da fila e o cancelamento seguro em excesso de eventos permanecem. Isso não comprova que a confiabilidade do toque físico foi resolvida.
- **Saída:** botão central **VOLTAR A GUILDA**, de **224 × 38**, nas telas de encerramento/indisponibilidade. Mantém a limpeza de 350 ms, liberação do rádio e volta ao retrato; o botão SAIR durante o combate permanece. Um dedo já pressionado não pode confirmar a saída ao mudar de tela.
- **Aceite:** o anfitrião tem **30 segundos a partir do primeiro pedido válido**, sem renovação por retransmissões. O aceite continua explícito e não é aceito no instante do vencimento. O convidado vê **PRAZO APROX.**, iniciado após a confirmação válida do anfitrião; atraso de rede pode fazê-lo terminar depois do prazo real, sem prolongar o convite. O limite local máximo nessa espera é menor que 42 segundos desde ENTRAR.
- **Mensagens distintas:** convite expirado, recusa manual, sala ocupada e tempo técnico de conexão esgotado. A conexão inicial e a sincronização após o aceite continuam com **12 segundos**. **Não há prazo humano para escolher ação, habilidade ou alvo em combate enquanto o link estiver saudável.**
- **Compatibilidade entre placas:** protocolo de convite **v2**; ambas precisam instalar coopfix77 para este treino. Isso é separado do protocolo PvP v5/52 bytes, que permanece inalterado.

O treino continua experimental, com dois jogadores, duas ondas e um chefe, em cópias somente em RAM: **sem XP, ouro, itens recebidos, consumo de recursos da campanha, alteração dos saves ou do RNG da campanha**. Não há mudança de transporte de combate, potência/canal/sleep do rádio, energia, SHOP, regras/recompensas, campanha, lore, PvP ou artes. Permanecem **Save30/160 bytes, leitura Save29/30, época 1 e 10 rações, sem novo reset**. A ressalva de downgrade abaixo continua válida.

### Qualificação concluída, separada da publicação

Fonte qualificada: [b688df55d8137ccdd9d70bd491579c309c67c5ad](https://github.com/Oflodorporto/RPG-POKET-2/commit/b688df55d8137ccdd9d70bd491579c309c67c5ad), tree `8aff875e2d54967744784fd390ddae0e45c60047`, parent `d767538669137ca08a2dd53f3805e8ba1be006b8`. [Run 38668885570](https://github.com/Oflodorporto/RPG-POKET-2/actions/runs/38668885570), job `116207035914`, artefato `11945006834`: aprovado no SHA exato, com auditoria independente dos logs e bytes.

- **153/153 grupos, incluindo preparação:** todos os 152 herdados, mais regressão de toque/convite/saída. Modos otimizado e **ASan/UBSan/LeakSanitizer obrigatório, `detect_leaks=1`**, sem substituir o CI por testes locais parciais.
- **ESP32-S3/core 3.3.12**, compilador esp-x32/2601: padrão Arduino **ON sem override** e build **OFF explícito** aprovados. Imagem ON qualificada de **2.956.464 bytes**, margem OTA **189.264 bytes** na partição de 3 MiB; RAM estática do linker **62.340 bytes**. São números da qualificação, com o mesmo tamanho final conferido na publicação; RAM estática não mede heap livre. ELF de alvo e imagem OFF não foram disponibilizados para rehash independente; os relatórios de compilação/seções/símbolos foram auditados, e os bytes ON foram conferidos diretamente.
- Renderer real: **1.183 frames por modo**, **58 pares herdados e 16 pares adicionais de convite**, com paridade exata de pixels/textos, desenho sem alocação e preservação do estado. Regressões: **98 casos de controlador de toque**, **4.000 comparações de sequências** e **45 cenários de convite por modo**. Capturas host não medem FPS nem validam hardware.
- OTA em adapters host sobre os bytes reais compilados, otimizada e sanitizada: **11 clientes históricos, incluindo o coop77 público**, e instalador de produção atual, com as unidades de tradução históricas/atuais separadas. Preservados Save29→30/Save30→30, três slots, época, Wi-Fi, arte e limites de ração, inclusive rejeição/fallback de leitores antigos.
- As **666 artes** continuam byte-idênticas: **7.470.928 bytes**, CRC32 `9b4bcfc0`. Permanecem 42 prévias PvP e três do relógio byte-idênticas, 14 pares de comércio e cobertura de narrativa de 3.481 frames/671 prévias. PSRAM conhecida **7.638.272 bytes** é contabilidade estática.

[Contrato de qualificação](COOPFIX77_RELEASE_QUALIFICATION.md) · [Notas da correção](RELEASE_COOPFIX77_NOTAS.txt). Esses arquivos preservam a etapa de preparação; o registro verificado da publicação está abaixo.

### Publicação e downloads públicos verificados

[Publisher 38674025405](https://github.com/Oflodorporto/RPG-POKET-2/actions/runs/38674025405), job `116223406787`: **concluído com sucesso em 15/10/2026 às 03:54:02 UTC**. A [Release coopfix77](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.15-coopfix77), ID `413399108`, foi publicada em **15/10/2026 às 03:49:43 UTC**. A tag e main, no fechamento da publicação, foram confirmados no commit qualificado `b688df55d8137ccdd9d70bd491579c309c67c5ad`, tree `8aff875e2d54967744784fd390ddae0e45c60047`; a Release latest aponta para a mesma entrega.

O publisher repetiu **153/153 grupos**, ASan/UBSan com **LeakSanitizer obrigatório, `detect_leaks=1`**, e ambos os builds ESP32 **default ON/override OFF**. Verificador e OTA com bytes reais foram aprovados antes da publicação e novamente sobre os downloads públicos, nos modos otimizado e ASan/UBSan/LSan, com os 11 clientes históricos e o instalador atual. A auditoria independente conferiu o artefato final `11947581047`, logs, snapshots remotos de fechamento e os downloads públicos; **não repetiu OTA nem sanitizadores localmente**.

Os três downloads foram conferidos por tamanho/SHA-256, comparados byte a byte com a saída final do publisher e confrontados com os digests da Release e o manifesto:

- `firmware.bin`: **2.956.464 bytes**, margem OTA **189.264 bytes**; SHA-256 `ffe65f8ecb71a2615e679c3d845bcb9c5a68071b254cedb1f37868cab0c989cd`.
- `artes.pak`: **7.470.928 bytes**; SHA-256 `14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56`; byte-idêntico às artes herdadas.
- `manifest.json`: **1.228 bytes**; SHA-256 `3de7a96d46187b108c8e9946d23325f7c879b7c7cb5f9051e73cc341bd421a64`.

Correspondência byte a byte de `releases/latest/download/manifest.json` com o manifesto da tag: **confirmada**. Os hashes públicos do firmware e do manifesto diferem dos hashes da qualificação, porque o publisher recompilou o mesmo SHA. A comparação independente encontrou **69 bytes diferentes**, restritos à hora de compilação, hash ELF do descritor e checksums/hashes da imagem; instruções e dados fora desses campos são idênticos. O manifesto mudou apenas no SHA-256 do firmware. Os valores acima pertencem aos downloads públicos.

A publicação não instala automaticamente. Nenhuma instalação USB ou OTA física, teste de rádio ou validação de toque foi realizada por estes passos de publicação e auditoria.

### Relatos físicos e reteste necessário

Na versão anterior coop77, o proprietário relatou que os dois aparelhos conseguiram sair depois de toques repetidos e que uma partida em duas placas chegou à vitória. Depois, a espera pelo aceite do anfitrião voltou a terminar em aproximadamente 12 segundos; esse caso foi reproduzido em software. Esses relatos não demonstram congelamento global permanente nem determinam a causa física do primeiro encerramento de conexão.

**O reteste físico da coopfix77 permanece pendente.** Conferir ambas as placas atualizadas, aceite perto do vencimento, expiração sem aceite, recusa, seleção demorada de habilidade/alvo, retorno central e saída/reentrada. RF, toque/orientação, restauração de Wi-Fi, USB/OTA físico, SD/saves, energia, heap/stack/PSRAM, latência e FPS continuam sem validação completa. Não houve ajuste de rádio/potência/sleep nem comprovação de correção física de alcance/desconexões. [Roteiro existente e complemento da correção](COOP_TRAINING_HARDWARE_CHECKLIST.md).

## Histórico preservado: coop77 · versão anterior

O registro a seguir descreve a entrega coop77 e sua situação em 14/10/2026. Referências a “atual”, “latest” e testes pendentes pertencem àquela etapa; os relatos físicos posteriores e a correção coopfix77 estão registrados acima. Os dados de publicação e qualificação originais são preservados.

### Firmware da entrega coop77

A qualificação de software, a publicação e os downloads públicos foram verificados no SHA exato. A validação física continua pendente. O registro de shop77 permanece preservado mais abaixo como histórico da versão anterior.

**Firmware atual:** 7.7 — Treino cooperativo experimental, `2026.10.14-coop77` / build `2026101406`, tag `v2026.10.14-coop77`. [Release](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.14-coop77) · [Uso e limites](RELEASE_PACKAGE12_NOTAS.txt) · [Qualificação do pacote](PACKAGE12_RELEASE_QUALIFICATION.md) · [Controles do treino](COOP_TRAINING_CONTROLS.md).

### Escopo do treino

Na Guilda, **Coop treino** abre um encontro experimental de **dois jogadores, duas ondas e um chefe**. Uma placa cria a sala, a outra escolhe o anfitrião, e o anfitrião confere e aceita o parceiro. A tela usa paisagem lógica 320 × 240 no framebuffer 240 × 320 existente; ao sair, retorna à Guilda em retrato. Não há segundo framebuffer nem novas artes.

Cada encontro usa **cópias dos personagens somente em RAM**: sem XP, ouro, recompensas, avanço de missão, consumo de poções da campanha, gasto do RNG da campanha ou alteração de seus saves. Ataque com alvo, **Foco teste** de 2 MP, Defender e poções da cópia RAM compõem as ações do treino. Foco teste não equivale ao conjunto completo de poderes de classe. Coop com progressão de campanha não está entregue.

Cancelar antes do envio não gasta recurso nem avança o turno. Depois de enviar, o controle aguarda confirmação, sem gasto especulativo do cliente. Saída ou queda prolongada encerra o treino, sem fabricar vitória, migração de anfitrião ou retomada durável. PvP, atualização e operações incompatíveis de Wi-Fi bloqueiam a entrada simultânea.

### Compatibilidade e conteúdo preservado

Mantém **comércio em lote, limite de 10 rações, preços e recompensas herdados, Save30 de 160 bytes, leitura Save29/30, três slots e época 1, sem novo reset**. Save29 migra apenas no próximo save normal; ler ou instalar não grava saves. A primeira passagem de touch77 ou anterior continua exigindo USB com `EraseFlash=none`, sem importar Save1…28. A OTA depende de confirmação no aparelho; publicar não instala automaticamente.

**Não há garantia de downgrade seguro:** o leitor shop77 aceita 0..10 rações, mas leitores anteriores à shop77, limitados a 9 rações, rejeitam registros com 10 rações; um journal misto pode voltar ao registro anterior com até 9 e perder progresso posterior, e dois registros com 10 bloqueiam a leitura antiga. Compartilhar Save30 não elimina essa diferença.

O **PvP v5/52 bytes**, a apresentação dos dois aventureiros e as regras de interrupção/liquidação permanecem; não há mudança para o protocolo anterior v4. A narrativa **Ecos de Valdaria**, lore, campanha, exploração procedural, relógio, SD, Wi-Fi e as **666 artes** permanecem. O pacote de arte público tem 7.470.928 bytes e CRC32 `9b4bcfc0`, conferido byte a byte com as artes herdadas. Pets e a nova bolsa visual não fazem parte desta entrega. O histórico abaixo preserva os detalhes de SHOP, PvP, Save30 e relógio.

### Qualificação de software concluída

Fonte qualificada: [da68148210b3155b72e41deb08a46ab854d09a0e](https://github.com/Oflodorporto/RPG-POKET-2/commit/da68148210b3155b72e41deb08a46ab854d09a0e), tree `6c003c6df520658297b832f6abc6e61645defd8c`. [Run 38645121720](https://github.com/Oflodorporto/RPG-POKET-2/actions/runs/38645121720), job `116129949303`: aprovado no SHA exato, com artefato e logs auditados independentemente.

- **152/152 checkpoints**, preservando os 149 herdados e acrescentando três grupos: coop ON/OFF, instalador real multi-TU e renderer/no-allocation. Modos otimizado e ASan/UBSan com **LeakSanitizer obrigatório, `detect_leaks=1`**.
- ESP32-S3/core **3.3.12** compilado com o padrão Arduino **ON sem override** e com override **OFF**. Somente o binário default ON compõe a release. A imagem ON qualificada tem **2.954.432 bytes**, com **191.296 bytes de margem OTA** na partição de 3 MiB; o linker registra **62.324 bytes de RAM estática**. Esses números descrevem a qualificação; os bytes públicos recompilados e seus hashes estão registrados abaixo. RAM estática não mede heap livre.
- Renderer real: **1.139 frames por modo**, **58 pares de capturas** e paridade de pixels otimizado/sanitizado, com desenho sem alocação e conservação de estado da campanha/Save30/RNG. São capturas host, não medidas de FPS ou legibilidade física.
- OTA em adapters host sobre os **bytes reais compilados**, nos dois modos, com clientes históricos e preservação de Save29→30/Save30→30, slots, época, Wi-Fi, arte e limite de 10 rações. Essa prova não equivale a OTA física em duas placas.
- As 42 prévias PvP e três prévias do relógio permanecem byte-idênticas à qualificação anterior; permanecem os 14 pares de comércio e a cobertura de narrativa de 3.481 frames/671 prévias. O orçamento conhecido de PSRAM, **7.638.272 bytes**, é contabilidade estática, não medição de memória livre.

A auditoria verificou diretamente os bytes ON e os relatórios de compilação/seções/símbolos. ELF de alvo e imagem OFF não foram incluídos no artefato para rehash independente. Isso não substitui os ensaios físicos pendentes.

### Publicação e downloads públicos verificados

[Publisher 38651255165](https://github.com/Oflodorporto/RPG-POKET-2/actions/runs/38651255165), job `116150329938`: **concluído com sucesso**, com registro final atualizado em **14/10/2026 às 23:49:49 UTC**. A [Release coop77](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.14-coop77), ID `413278656`, foi publicada em **14/10/2026 às 23:45:55 UTC**. A tag resolve para o commit qualificado `da68148210b3155b72e41deb08a46ab854d09a0e`, também confirmado em main no fechamento da publicação; a Release latest aponta para a mesma entrega.

O publisher repetiu **152/152 checkpoints**, ASan/UBSan com **LeakSanitizer obrigatório**, ambos os builds ESP32 **default ON/override OFF** e OTA com os bytes reais finais, em modos otimizado e sanitizado. Depois de publicar, repetiu o verificador e a OTA em ambos os modos sobre os downloads públicos. Todos esses passos foram aprovados no CI do SHA exato. A auditoria independente final confirmou os artefatos e os bytes públicos e repetiu a OTA em adapters host no modo otimizado, com resultado aprovado.

Os três downloads obtidos separadamente foram conferidos por tamanho/SHA-256, comparados byte a byte com a saída final do publisher e confrontados com os digests da Release e o manifesto:

- `firmware.bin`: **2.954.432 bytes**, margem OTA **191.296 bytes**; SHA-256 `fed875fd458fb1c23d7b0dca4a525e35314ad50a743547c4ffc3989e31e9fab3`.
- `artes.pak`: **7.470.928 bytes**; SHA-256 `14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56`; byte-idêntico às artes herdadas.
- `manifest.json`: **1.208 bytes**; SHA-256 `dd97852974419c5f08e33652367dbc2e9c89e522e3bba2805cc6d4fe8e8d0324`.

Correspondência byte a byte de `releases/latest/download/manifest.json` com o manifesto da tag: **confirmada**. Os hashes públicos do firmware e do manifesto são diferentes dos hashes da qualificação, pois o publisher recompilou o mesmo SHA. Os valores acima pertencem aos downloads públicos, não ao candidato da qualificação. Nenhuma instalação USB ou OTA física foi realizada por esta publicação.

### Hardware ainda não validado

**Permanecem pendentes** duas placas reais: descoberta/aceite e RF, distância/perda de ACK/reconexão, saída/reentrada, restauração Wi-Fi/ESP-NOW, coexistência com PvP, toque nos quatro cantos, orientação e legibilidade, relógio a 10% de backlight, USB/OTA físico, SD, três slots, cortes reais de energia, latência/consumo e medições de heap/stack/PSRAM/FPS. Compilação, screenshots host e publicação não comprovam esses pontos. O teste de pvp77 relatado anteriormente não transfere validação física ao coop77.

O [roteiro de duas placas](COOP_TRAINING_HARDWARE_CHECKLIST.md) permanece como documento histórico de preparação: suas referências a flag OFF e ausência de binário qualificado descrevem aquela etapa. Para Package12, a identidade e o padrão Arduino ON são os registrados nesta qualificação. O roteiro ainda não foi executado.

`PACKAGE12_STAGING_ONLY.txt`, `RELEASE_PACKAGE12_NOTAS.txt` e `PACKAGE12_RELEASE_QUALIFICATION.md` conservam o registro da preparação anterior à publicação. Os resultados de software e os bytes públicos verificados desta entrega estão registrados acima. Testes automatizados e publicação não constituem validação física.

## Histórico preservado: shop77 · versão anterior

O registro abaixo descreve a entrega shop77. Referências a “esta versão”, “atual” ou coop ainda separado pertencem a esse registro histórico, não ao escopo de treino experimental de coop77 acima.

**Firmware anterior: 7.7 — Comércio em lote, 2026.10.14-shop77 / build 2026101405.** [Release](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.14-shop77) · [Uso e instalação](RELEASE_PACKAGE11_NOTAS.txt) · [Qualificação e limites](PACKAGE11_RELEASE_QUALIFICATION.md). Identidade do pacote: `7.7 - Comercio em lote`. Fonte/tag [ec7efc08](https://github.com/Oflodorporto/RPG-POKET-2/commit/ec7efc085ffec6e583e35cbafea5a3f4208219b1), tree `2fb195d10ff2b23f2373ac2e75d37aaba6f39039`. [Qualificação](https://github.com/Oflodorporto/RPG-POKET-2/actions/runs/38619267270): sucesso no SHA exato, com artefato e logs auditados independentemente. Publicação e downloads: [run 38624539079](https://github.com/Oflodorporto/RPG-POKET-2/actions/runs/38624539079), concluído com sucesso em 14/10/2026 às 19:59 UTC; Release publicada às 19:58 UTC. Correspondência da tag com o SHA qualificado: confirmada no commit `ec7efc085ffec6e583e35cbafea5a3f4208219b1`. Conferência independente de tamanho/SHA-256 dos três downloads, digests da Release e manifesto: aprovada. Correspondência byte a byte de latest com o manifesto da tag: confirmada. Os hashes abaixo correspondem aos downloads públicos recompilados, diferentes dos hashes do firmware e manifesto da qualificação.

Na loja, escolha a quantidade para **comprar ou vender de uma vez** e confira o total antes de confirmar. A seleção respeita o saldo, o estoque, a capacidade de cada item e o limite de ouro na venda. **Rações agora cabem até 10**; os demais limites, preços e recompensas permanecem iguais. Ouro e itens do lote são tratados juntos na gravação. Cancelar antes de confirmar não conclui a operação; toques repetidos não repetem o lote. Se a gravação falhar, siga a mensagem na tela e use **Tentar salvar**; não desligue durante o salvamento. A compra do kit necessária para recuperar uma escolta permanece disponível.

Duelo com aparência completa dos dois aventureiros, HP/MP, guarda, custos de mana e animações de ações confirmadas. Desistir também funciona no turno do adversário. **As duas placas precisam do protocolo v5/52 bytes**, preservado de pvp77 e incompatível com o v4/44 anterior; a tela pede para atualizar ambas quando encontra um protocolo incompatível. Sem troca de protocolo durante uma partida.

**Depois do início**, uma interrupção sem prova confirmada do resultado termina em **NAO CONFIRMADO**, sem prêmio de vitória, mantendo consumida a entrada já utilizada. **Antes do início**, permanece a devolução integral existente. Em desistência simultânea, prevalece a solicitação de quem possui o turno acordado; o outro pode ganhar mesmo tendo solicitado desistência.

**Save30 de 160 bytes: grava 30, lê 29/30 e mantém época 1, sem novo reset.** A versão nova lê os registros anteriores com até 9 rações e os novos com 10. Save29 migra somente no próximo save normal; leitura e instalador não gravam saves. A primeira passagem de touch77 ou anterior continua exigindo USB com `EraseFlash=none`, sem importação de Save1..28.

**Não há garantia de downgrade seguro.** Os leitores históricos rejeitam registros com 10 rações. Num journal misto, podem recuperar um registro anterior com até 9, perdendo as alterações posteriores; se ambos os registros tiverem 10, a leitura antiga fica bloqueada, sem escrita nem limpeza. O mesmo número Save30 não garante que o firmware antigo aceite o novo limite. A qualificação verifica separadamente Save29/30 nos três slots, preservação de progresso/RNG e rejeição/fallback pelos leitores históricos: aprovada nos modos otimizado e sanitizado, incluindo cinco leitores Save30 históricos, cada um com 450 casos compatíveis e 45 casos de rejeição/fallback por modo.

O agregado completo desta versão contém **149 tarefas, incluindo preparação: 143 herdadas mais seis de comércio/rações e leitores antigos**, sem retirar ou enfraquecer testes antigos. Resultado final, modos otimizados e ASan/UBSan com LeakSanitizer obrigatório no CI: 149/149 aprovadas na qualificação e novamente no publisher, com `detect_leaks=1`. Compilação ESP32-S3/core 3.3.12 com transporte ESP-NOW/FreeRTOS real e pacote de 52 bytes: aprovada nas duas execuções; sketch de 2.909.426 bytes e 55.748 bytes de variáveis globais estáticas, sem equivalência a heap livre medido. OTA em adapters host com os bytes reais compilados, em modos otimizado e sanitizado, incluindo a transição pelo cliente histórico exato de pvp77, correspondente à versão pública: aprovada nos dois modos, com preservação dos slots, época, Wi-Fi e arte anterior. Isso não equivale a instalação física nas placas. A repetição do agregado, recompilação do mesmo SHA e OTA com os bytes finais da publicação: aprovadas, incluindo comparação byte a byte dos três downloads com a saída recompilada e do manifesto latest com a tag.

A evidência da versão anterior pvp77 registrou 245.760 transições no trace pareado, 596 capturas comparadas entre os modos e 42 prévias finais de PvP, incluindo 12 GIFs. A nova qualificação mantém a cobertura herdada e acrescenta capturas de comércio: 14 pares de frames de comércio idênticos entre os modos; 42 prévias PvP e três do relógio idênticas à qualificação anterior; narrativa com 3.481 frames, 671 prévias e 854 registros de consulta preservados. Das 671 imagens de narrativa, 665 são byte-idênticas e as seis diferenças correspondem somente às telas de compra adaptadas ao lote. As imagens vêm do renderer host; sua amostragem não mede FPS do ESP32.

Na evidência da versão anterior pvp77, o journal de produção Save30/época 1 passou falhas de escrita/read-back, confirmação falsa de escrita, retry e reboot. Nas **dez fronteiras simuladas de corte de energia na liquidação, sete recuperaram perda conservadora e três a vitória durável**, sem crédito duplicado. Os testes sustentam crédito **no máximo uma vez**, não liquidação atômica ou exatamente uma vez: uma vitória confirmada somente em RAM ainda pode ser perdida antes do commit durável. Os 48 cortes herdados pertencem ao Save28 antigo, não à cobertura do Save30 atual. Não há consenso durável nem recuperação garantida do resultado por reconexão.

O pulso do relógio e a narrativa **Ecos de Valdaria**, regras/recompensas PvE e geração procedural permanecem preservados. A nova bolsa visual, pets e coop são frentes separadas e não estão integradas neste pacote. As 666 artes permanecem byte-idênticas, com 7.470.928 bytes e CRC32 `9b4bcfc0`, sem novo atlas, framebuffer ou chamada de alocação de produção. O orçamento conhecido de PSRAM, **7.638.272 bytes**, é contabilidade estática, não medição de heap livre. **A validação física completa de shop77, incluindo PvP/RF entre duas placas, visibilidade real do relógio a 10% de backlight, toque, USB/OTA físico, FPS, latência, consumo, RAM livre e cortes reais de energia, continua pendente.** O proprietário relatou teste de pvp77 fluido em 14/10/2026; isso não equivale à validação física completa de shop77. Testes host e compilação não substituem essa validação.

Arquivos públicos recompilados e conferidos por tamanho/SHA-256:
- `firmware.bin`: 2.909.568 bytes, margem OTA 236.160 bytes; SHA-256 `0dada763a209eb9ca3658de59d2ed8a51ee434631576994f914325db33e7abbe`.
- `artes.pak`: 7.470.928 bytes; SHA-256 `14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56`.
- `manifest.json`: 1.193 bytes; SHA-256 `b923ba67ff4553a1890dc07f4a2a233f21864c9e775a5fb8d1d1bfaf6afdcbfd`.

Os arquivos versionados `PACKAGE11_STAGING_ONLY.txt`, `docs/RELEASE_PACKAGE11_NOTAS.txt` e `docs/PACKAGE11_RELEASE_QUALIFICATION.md` conservam o registro da preparação anterior à publicação. O resultado verificado desta entrega está nos registros e hashes acima. Os documentos do Pacote 10 permanecem como histórico de pvp77. Testes automatizados e publicação não constituem validação física.

