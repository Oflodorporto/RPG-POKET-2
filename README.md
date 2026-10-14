# RPG POKET 2.0

RPG para Waveshare ESP32-S3-Touch-LCD-2, SKU 29667 (16 MiB flash, 8 MiB OPI PSRAM, tela 240×320).

## Instalação inicial

Abra `firmware/RPG_POKET_2/RPG_POKET_2.ino` no Arduino IDE. Core Espressif 3.3.12, ESP32S3 Dev Module, flash 16 MB, PSRAM OPI, partição `app3M_fat9M_16MB`, USB CDC ativado. Não apague a flash. Para instalar as artes pelo computador, baixe `artes.pak` da Release e coloque em `/RPGPOKET/artes.pak` num microSD FAT32. No repositório, `tools/unpack_art.py` reconstrói o pacote a partir das fontes compactadas e o workflow o valida; para instalar, use o `artes.pak` final da Release. A passagem de touch77 ou anterior para scenes77/Save29 também exige uma instalação pelo cabo com `EraseFlash=none`: os três slots de jogo são reiniciados uma única vez, preservando Wi-Fi e artes. Não há importação de saves antigos. Veja [instruções Save29/USB](docs/RELEASE_SCENES_7_7_NOTAS.txt).

## Atualizações pela placa

Conecte a uma rede 2,4 GHz. Vá a **Configurações → Atualização → Verificar versão**. Confirme **Instalar** para baixar o firmware e as artes. A placa confere SHA-256, tamanho e CRC do pacote de artes, instala o firmware na partição livre e reinicia. Downloads completos com falha de leitura podem ser revalidados sem novo download. Há uma única remontagem de recuperação a 1 MHz, sempre reiniciando a validação integral. Após sucesso, temporários são removidos; pacotes anteriores e backup são retidos, ocupando espaço adicional. Após a instalação inicial Save29 e seu reset único autorizado, as atualizações compatíveis preservam os três saves em NVS. O cliente touch77 recusa o manifesto Save29-only; não tente essa primeira passagem por OTA.

Sem uma Release publicada, a placa informa que ainda não há versão disponível. Se as artes locais faltarem, é possível baixar as da mesma versão sem regravar o firmware. Não há leituras periódicas do cartão durante o jogo. Não retire o cartão ou a alimentação durante a instalação. FAT32 não garante recuperação de toda falha de energia; não se promete rollback automático de firmware que não inicialize.

## Publicar uma versão

O Pacote 8 exige qualificação bem-sucedida do mesmo SHA em `feat/package8-release`, identidade final exata e tag inédita; usa `verify_package8_release.py` e `test_package8_release.py`, mantendo os verificadores históricos. Confira o [contrato de qualificação](docs/PACKAGE8_RELEASE_QUALIFICATION.md). A promoção de main e a publicação exigem aprovação explícita do proprietário. Para uma versão futura, revisar explicitamente o verificador e o gate de qualificação; mudar apenas a versão não basta.

1. Revise `ReleaseVersion.h`, com versão distinta e `FW_BUILD` crescente, e os contratos de saves, artes e memória.
2. Congele as fontes revisadas e mantenha o inventário e os verificadores da versão consistentes.
3. Envie à branch de qualificação e aguarde todos os testes, sanitizadores, compilação ESP32 e OTA com bytes reais do SHA exato.
4. Com autorização de publicação, promova esse mesmo SHA a `main`, sem um merge que altere a fonte qualificada. O workflow aceita somente push em main, sem publicação manual por Run workflow.
5. O workflow repete os gates, recompila, publica `firmware.bin`, `artes.pak` e `manifest.json`, e confere os downloads públicos e latest. Alterações apenas neste README não iniciam compilação/publicação. O hash público é o do novo build, conferido após a publicação.

## Estado atual

**Firmware atual: 7.7 — Ecos de Valdaria, 2026.10.14-story77 / build 2026101402.** [Release](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.14-story77) · [Uso e instalação](docs/RELEASE_PACKAGE8_NOTAS.txt) · [Qualificação e limites](docs/PACKAGE8_RELEASE_QUALIFICATION.md). Fonte/tag [9ca94294](https://github.com/Oflodorporto/RPG-POKET-2/commit/9ca94294daf185ac80603e3ba409f0201eef72fb); [qualificação](https://github.com/Oflodorporto/RPG-POKET-2/actions/runs/38533435084) e [publicação/downloads](https://github.com/Oflodorporto/RPG-POKET-2/actions/runs/38557877229) aprovadas. Publicado em 14/10/2026. Os três downloads públicos foram conferidos independentemente por tamanho/SHA-256 contra os digests da Release e o manifesto; latest é byte-idêntico e a tag aponta ao SHA qualificado exato.

Revisão narrativa aprovada: diálogos coerentes com o avanço da história, doze reações individuais ao final pleno, retornos das quatro origens, objetivos e cartas mais claros, voz em primeira pessoa e distinção entre a primeira e novas expedições. A carta final de Sabela apresenta Neris e Talassar preservando a vitória de Aeldra. Não implementa segundo mapa, nova missão, recompensa ou flag. Mecânicas, RNG, custos, balanceamento e PvP permanecem preservados. O reforço da visibilidade da animação do relógio está separado para uma atualização posterior; não faz parte deste pacote.

**Save30: lê 29/30 e grava 30, mantendo época 1 e sem novo reset.** De scenes77, scale77, camp77, explore77 ou immersion77, atualizar por **Configurações → Atualização → Verificar versão → Instalar**, em Wi-Fi 2,4 GHz. A leitura e o instalador não gravam saves; Save29 migra somente no próximo save normal. Os três slots, marcador Done, seed/run procedural, pisos, máscaras, posições e próximo RNG são preservados. Leitores Save29 antigos recusam slots Save30 sem sobrescrevê-los. Os leitores/journals explore77 e immersion77 aceitam novamente os Save30 gravados pelo pacote8 em leitura estritamente sem escrita; isso prova compatibilidade do codec, não permite downgrade do firmware pelo OTA. A primeira passagem de touch77 ou anterior continua exigindo USB com `EraseFlash=none`, sem importação de Save1..28.

Os 130 grupos do agregado completo passaram com ASan/UBSan e LeakSanitizer; 911 fontes conferidas no snapshot qualificado. O workflow da publicação repetiu os gates, recompilou o mesmo SHA e verificou OTA também com os downloads públicos. Cobertura dos três slots: 30 probes de recusa Save30 em cada leitor Save29 de scenes77/scale77/camp77 e 45 posicionamentos de read-back em cada leitor Save30 de explore77/immersion77. As 671 páginas PNG únicas do CI são byte-idênticas às prévias narrativas aprovadas, com 3.481 frames, 43.123 verificações de rótulos e 121 hashes de fontes de captura. Essas verificações não equivalem à inspeção humana de cada frame.

Firmware publicado 2.898.480 bytes, margem OTA 247.248 bytes; core ESP32 3.3.12 / Arduino CLI 1.5.2-rc.1 / flash 16 MiB / OPI PSRAM 8 MiB / partição 3 MiB. Artes666 inalteradas, 7.470.928 bytes. Nenhum novo atlas ou chamada de alocação; PSRAM conhecida estimada em 7.638.272 bytes. **Teste físico do story77, toque, heap livre, FPS, latência e falhas de energia na placa pendentes.** Orçamento estático e testes host não substituem medição ou validação física. A sobreposição visual herdada da linha de custo com alguns retratos de contribuição não foi alterada nesta revisão.

SHA-256 dos arquivos publicados no story77:
- `firmware.bin`: `b52f98101fcd75414be57beb935696f4daeb9b8138de3938a7df72cc0f18dd7b`
- `artes.pak`: `14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56`
- `manifest.json`: `a4eb17800562f2cdb830c6ae8dd2c6026cbea6f47a9f1c22e1bd4ba7df4c0212` (1.094 bytes)

O arquivo `PACKAGE8_STAGING_ONLY.txt` e os documentos de candidato/qualificação registram a etapa anterior à publicação; suas restrições de candidato são históricas. O estado publicado vigente está descrito acima. A publicação desta versão não autoriza automaticamente versões seguintes.

**Entrega anterior: 7.7 — Imersão nas Ruínas, 2026.10.14-immersion77 / build 2026101401.** [Release](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.14-immersion77) · [Uso e instalação](docs/RELEASE_PACKAGE7_NOTAS.txt) · [Qualificação e limites](docs/PACKAGE7_RELEASE_QUALIFICATION.md). Fonte/tag [e775b5d1](https://github.com/Oflodorporto/RPG-POKET-2/commit/e775b5d1854174b45404ca0cd3305c1e24610861); [qualificação](https://github.com/Oflodorporto/RPG-POKET-2/actions/runs/38518714858) e [publicação/downloads](https://github.com/Oflodorporto/RPG-POKET-2/actions/runs/38521825552) aprovadas. Publicado em 14/10/2026. Os três downloads públicos foram conferidos independentemente por tamanho/SHA-256; o manifesto latest é byte-idêntico e a tag aponta ao SHA qualificado exato.

Ruínas medievais redesenhadas, com duas páginas de quatro botões. Refeição sentada personalizada por raça/classe/cores, com correção pontual do texto de erro do acampamento. Relógio com sol/lua animados pela hora local e repintura parcial. Fadiga PvE aplicada uma vez por ação: −1 de dano em 71–89 e −2 em 90–100. Dados reais, recibos, geração procedural R5, D20 e Bolsa das entregas anteriores preservados. A auditoria de narrativa é separada e não altera a história deste pacote.

**Save30: lê 29/30 e grava 30, mantendo época 1 e sem novo reset.** De scenes77, scale77, camp77 ou explore77, atualizar por **Configurações → Atualização → Verificar versão → Instalar**, em Wi-Fi 2,4 GHz. Os três slots, marcador Done, seed/run procedural, pisos, máscaras, posições e próximo RNG são preservados. Save29 migra somente no próximo save normal; a leitura e o instalador não gravam saves. Leitores Save29 antigos recusam slots Save30 sem sobrescrevê-los. O leitor/journal explore77 aceita novamente os Save30 gravados pelo pacote7, preservando os dados; essa compatibilidade de codec não libera downgrade de firmware pelo OTA. A primeira passagem de touch77 ou anterior continua exigindo USB com `EraseFlash=none`, sem importação de Save1..28.

Os 127 grupos do agregado completo, incluindo as quatro novas áreas, integração e regressões herdadas, passaram no CI com ASan/UBSan e LeakSanitizer. Transições com os bytes compilados e depois publicados, três slots, recusa histórica e 45 combinações de read-back explore77 aprovadas. Das capturas conferidas, 928 são byte-idênticas às locais congeladas; outras 38 imagens das Ruínas reproduzem exatamente os pixels nativos e os layouts de ampliação declarados. Firmware publicado 2.891.632 bytes, margem OTA 254.096 bytes, core ESP32 3.3.12 / flash 16 MiB / OPI PSRAM 8 MiB / partição 3 MiB. Artes666 inalteradas, 7.470.928 bytes. Novo atlas: 18.144 bytes constantes em flash, sem nova alocação PSRAM; PSRAM conhecida estimada em 7.638.272 bytes. **Teste físico do immersion77, toque, heap livre, FPS, latência e falhas de energia na placa pendentes.** Testes host não substituem validação física.

SHA-256 dos arquivos publicados no immersion77:
- `firmware.bin`: `3c589ddebe0d4b0de00a0daefa4907ca47ce143ca8fbd64bc45501d94ff3b7d7`
- `artes.pak`: `14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56`
- `manifest.json`: `e46ec7455be349672fcef08420145d61b9c7314d452fcc4f978c869754df4301`

O arquivo `PACKAGE7_STAGING_ONLY.txt` e os documentos de candidato/integração registram a etapa anterior à publicação; suas restrições locais são históricas. O estado publicado vigente está descrito acima.

**Entrega anterior: 7.7 — Dados e exploração das Ruínas, 2026.10.13-explore77 / build 2026101309.** [Release](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.13-explore77) · [Uso e instalação](docs/RELEASE_PACKAGE6_NOTAS.txt) · [Qualificação e limites](docs/PACKAGE6_RELEASE_QUALIFICATION.md). Fonte/tag [3f9d3221](https://github.com/Oflodorporto/RPG-POKET-2/commit/3f9d3221b99e6b4ba55dc270a2a1d0a9155ea27b); [qualificação](https://github.com/Oflodorporto/RPG-POKET-2/actions/runs/38494326193) e [publicação/downloads](https://github.com/Oflodorporto/RPG-POKET-2/actions/runs/38500805596) aprovadas. Publicado em 14/10/2026. Os três downloads públicos foram conferidos independentemente por tamanho/SHA-256; o manifesto latest é byte-idêntico e a tag aponta ao SHA qualificado exato.

Dados e recibos refletem as rolagens reais do combate, com pulo da apresentação sem repetir ações/RNG. A faixa animada cobre dados explícitos de dano/cura; variação de arma, crítico e esquiva estão nos recibos Dados. Gerador R5 nas duas plantas de novas entradas pagas das Ruínas, com caminhos/encontros iniciais variados e limites documentados de dificuldade, cura e recompensas. Ilha e expedições de cartas continuam com mapas fixos. D20, acampamento personalizado, tenda e Bolsa nas Ruínas preservados; arte666 inalterada.

**Save30: lê 29/30 e grava 30, mantendo época 1 e sem novo reset.** A leitura de Save29 não grava; cada slot migra no próximo save normal. Runs antigos preservam mapa, piso, posição, máscaras e RNG até terminar. Firmware Save29 antigo bloqueia slots migrados sem apagá-los nem retomar silenciosamente uma cópia anterior; para reabrir é necessário firmware compatível Save30. De scenes77, scale77 ou camp77, atualizar por **Configurações → Atualização → Verificar versão → Instalar**, em Wi-Fi 2,4 GHz. A primeira passagem de touch77 ou anterior continua exigindo USB com `EraseFlash=none`, sem importação de Save1..28.

Agregado herdado: 91 suítes otimizadas e 50 sanitizadas mais todas as suites procedurais, trace, diferencial, integração e regressões Pacote5 aprovadas; ASan/UBSan e LeakSanitizer habilitados no CI. Revisão de 9.216 fixtures Save29 e falhas de journal/NVS; transições reais com os bytes publicados e 90 probes de rollback por leitores históricos aprovados. As 47 capturas procedurais/trace/combinadas do CI são byte-idênticas às locais congeladas. Firmware publicado 2.857.904 bytes, margem OTA 287.824 bytes, core ESP32 3.3.12 / flash 16 MiB / OPI PSRAM 8 MiB / partição 3 MiB. PSRAM conhecida estimada em 7.638.272 bytes. **Teste físico do explore77, toque, heap livre, FPS, latência e falhas de energia na placa pendentes.** Testes host não substituem validação física.

SHA-256 dos arquivos publicados no explore77:
- `firmware.bin`: `54a2c13b359dcfce3c32f9be9a8f01d350aab99fedae3350aabf98a6403b7cc3`
- `artes.pak`: `14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56`
- `manifest.json`: `4e4eb9eec4707bee35308f6a827a15f39f88432bcd1d7248e556886a5f912100`

Os arquivos `PACKAGE6_STAGING_ONLY.txt` e os documentos de candidato/integração registram a etapa anterior à publicação; suas restrições locais são históricas. O estado publicado vigente está descrito acima.

**Entrega anterior: 7.7 — D20, acampamento e bolsa, 2026.10.13-camp77 / build 2026101308.** [Release](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.13-camp77) · [Uso e instalação](docs/RELEASE_PACKAGE5_NOTAS.txt) · [Qualificação e limites](docs/PACKAGE5_RELEASE_QUALIFICATION.md). Fonte/tag [7a05617c](https://github.com/Oflodorporto/RPG-POKET-2/commit/7a05617cd1e13843d73de72f970c57a7113af48a); [qualificação](https://github.com/Oflodorporto/RPG-POKET-2/actions/runs/38486164852) e [publicação/downloads](https://github.com/Oflodorporto/RPG-POKET-2/actions/runs/38489809271) aprovadas. Publicado em 14/10/2026. Os três downloads públicos foram conferidos independentemente por tamanho/SHA-256, e o manifesto latest é idêntico.

D20 geométrico com vinte faces, acampamento comum com raça/classe/cores do personagem, tenda proporcional no mapa e botão Bolsa nas Ruínas. Bibliotecas reutilizáveis de equipamentos e efeitos preservadas como fontes; artes666 continuam idênticas. 96 suítes otimizadas e 55 com ASan/UBSan, com LeakSanitizer habilitado no CI; 68 capturas PNG/GIF do CI byte-idênticas às prévias congeladas. Compilação ESP32 3.3.12 do SHA exato e transições com os arquivos publicados aprovadas.

De scenes77 ou scale77/Save29, atualizar por **Configurações → Atualização → Verificar versão → Instalar**, em Wi-Fi 2,4 GHz. Os três slots e o marcador Done da mesma época1 são preservados byte a byte, sem novo reset; Wi-Fi e artes válidas também permanecem. A primeira passagem de touch77 ou anterior continua exigindo USB com `EraseFlash=none`, sem importação de Save1..28. Firmware publicado de 2.839.488 bytes, margem OTA de 306.240 bytes; PSRAM conhecida estimada em 7.638.272 bytes. **Teste físico rápido do camp77 relatado pelo proprietário sem detalhamento de cenários; medições de heap livre, FPS e ensaios prolongados continuam pendentes.** Registro detalhado das rolagens de combate e geração procedural de dungeon não fazem parte deste pacote.

SHA-256 dos arquivos publicados no camp77:
- `firmware.bin`: `81670f9aa9383e64e86ee65460031680ace898ef3732e280bb36d305a9893562`
- `artes.pak`: `14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56`
- `manifest.json`: `a8607abda195ef0064eeacb2178cc6f01f0c10cdc30069f76f4acf37d18eb9c1`

**Entrega anterior: 7.7 — Escala e contraste dos inimigos, 2026.10.13-scale77 / build 2026101307.** [Release](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.13-scale77) · [Uso e instalação](docs/RELEASE_SCALE_7_7_NOTAS.txt) · [Qualificação e limites](docs/SCALE77_RELEASE_QUALIFICATION.md). Fonte/tag [ad8acd8b](https://github.com/Oflodorporto/RPG-POKET-2/commit/ad8acd8b60f8c4c8bf961f87f8300445ad9d6be5); [qualificação](https://github.com/Oflodorporto/RPG-POKET-2/actions/runs/38438818461) e [publicação/downloads](https://github.com/Oflodorporto/RPG-POKET-2/actions/runs/38464942766) aprovadas. Os três arquivos públicos foram baixados e conferidos independentemente por tamanho/SHA-256 contra os digests da Release; firmware/artes também conferem com o manifesto, e o manifesto latest é idêntico.

Escala por tamanho da criatura e contraste em combate/dungeon, reutilizando as mesmas 666 artes de scenes77. Firmware publicado de 2.833.184 bytes, margem OTA de 312.544 bytes; artes de 7.470.928 bytes, CRC32 `9b4bcfc0`. De scenes77/Save29, atualizar por **Configurações → Atualização → Verificar versão → Instalar**, em Wi-Fi 2,4 GHz. Save29 e os três slots são preservados, com a mesma época 1 e seu marcador Done, sem repetir o reset; Wi-Fi e artes válidas também são preservados. A regressão de host com os arquivos publicados confirmou a transição scenes77 → scale77 e a preservação byte a byte dos três saves e do marcador. A primeira passagem de touch77 ou anterior continua exigindo USB com `EraseFlash=none`, sem importação de Save1..28. **Teste físico de scale77 na placa pendente.**

SHA-256 dos arquivos publicados:
- `firmware.bin`: `6d7b6b1b3da2f9231631a8b1fe34c97c8634f465cad29fa434032e91fb7fdc66`
- `artes.pak`: `14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56`
- `manifest.json`: `6c8c0eae1d18e4916e0d237fd88470fea3dd43a33dfc7bcc0e50c021c14f2b26`

**Entrega anterior: 7.7 — Cenários e viagem diária, 2026.10.13-scenes77 / build2026101306.** [Release](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.13-scenes77) · [Uso e instalação](docs/RELEASE_SCENES_7_7_NOTAS.txt) · [Qualificação e limites](docs/SCENES77_RELEASE_QUALIFICATION.md). Fonte/tag [dd626414](https://github.com/Oflodorporto/RPG-POKET-2/commit/dd626414b62bf6f88638ffe5d542196230c51061); [qualificação](https://github.com/Oflodorporto/RPG-POKET-2/actions/runs/38423671039) e [publicação/downloads](https://github.com/Oflodorporto/RPG-POKET-2/actions/runs/38428417889) verdes, três arquivos públicos e manifesto latest conferidos. 89 suítes otimizadas e 48 com ASan/UBSan, incluindo Save29, reset, viagens, controles e regressões; detecção de vazamentos habilitada no CI. As 97 capturas do CI são idênticas às prévias locais revisadas.

Relógio medieval, cidades e pré-partida regionais, derrota personalizada, aviso de nível e viagem diária com pernoite, caravana e tenda até Continuar. Save29-only, reset único dos três slots pela época fixa 1, preservando Wi-Fi e artes. **A primeira passagem de touch77 ou anterior exige cabo USB, sem apagar toda a flash; não há importação dos saves antigos.** Firmware publicado de 2.830.480 bytes, margem de 315.248 bytes na partição de 3 MiB; artes666 com os primeiros 636 assets preservados. PSRAM conhecida estimada em 7.638.272 bytes, sem medição de heap livre ou FPS. **Teste físico na placa pendente.**

**Entrega anterior: 7.7 — Bolsa e controles de toque, 2026.10.13-touch77 / build2026101305.** [Release](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.13-touch77) · [Evidências](docs/BOLSA_TOQUE_7_7.txt) · [Uso](docs/RELEASE_TOUCH_7_7_NOTAS.txt). Fonte/tag `91c64a36`; [qualificação](https://github.com/Oflodorporto/RPG-POKET-2/actions/runs/38374882773) e [publicação/downloads](https://github.com/Oflodorporto/RPG-POKET-2/actions/runs/38375799485) verdes, latest conferido pelo publicador. Bolsa de couro com nove células52×52, seleção separada de Usar/Ver, Equipamentos e Carvalho com controles maiores e textos locais mais grossos. Onze suites/cinco sanitizadores,9.234 casos de toque,2.112 frames novos, regressões de dungeon e transições reais aprovados. Firmware2.728.432 bytes/margem417.296; textura compacta na flash, artes externas636 inalteradas. Inclui integralmente SD/recovery e dungeon/Continue dos pacotes1/2. **Teste físico desta interface pendente.** Escala/contraste dos inimigos e demais frentes futuras não incluídos.

**Entrega anterior: 7.7 — Dungeon e interface de combate, 2026.10.13-dungeon77 / build2026101304.** [Release](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.13-dungeon77) · [Evidências](docs/CORRECOES_POS_TESTE_7_7.txt) · [Uso](docs/RELEASE_DUNGEON_7_7_NOTAS.txt). Fonte/tag `59258cec`; [qualificação](https://github.com/Oflodorporto/RPG-POKET-2/actions/runs/38368462066) e [publicação/downloads](https://github.com/Oflodorporto/RPG-POKET-2/actions/runs/38369185601) verdes, latest conferido. Continuar visível após vitória/derrota/fuga, estado verde acima das setas, HP/Ficha separados e Guia fora da mensagem. Oito suites/três sanitizadores,1.440 continuidades,136 toques,560 frames, compilação ESP32 e transições reais de artefatos aprovados. Firmware2.687.040 bytes/margem458.688. Recovery77, artes636, Save28, recompensas e RNG preservados. **Teste físico desta interface pendente.** Bolsa/Carvalho e escala/contraste dos inimigos não incluídos.

**Entrega anterior: 7.7 — Recuperação SD e instalação, 2026.10.13-recovery77 / build 2026101303.** [Release](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.13-recovery77) · [Evidências e limites](docs/OTA_RECUPERACAO_7_7.txt) · [Uso](docs/RELEASE_RECOVERY_7_7_NOTAS.txt). Fonte/tag `cf0639eb`; [qualificação](https://github.com/Oflodorporto/RPG-POKET-2/actions/runs/38365055106) e [publicação/downloads](https://github.com/Oflodorporto/RPG-POKET-2/actions/runs/38365745447) aprovadas, incluindo manifesto latest e segunda conferência independente. Recuperação única de leitura, staging completo preservado, SHA integral reiniciado e promoção com backup. 11 suítes focadas/5 alvos ASan/UBSan/LeakSanitizer, 27 cenários de recuperação, 19 de boot, 1.611 frames, 58 gates de catálogo por modo e transições 7.6/ota77. Firmware 2.686.416 bytes; margem OTA 459.312; artes636, Save28 e três slots preservados. **Esta recuperação ainda precisa de teste físico.** No firmware anterior ota77, o proprietário confirmou reparo completo e jogo com SanDisk64GB; o cartão novo4GB teve EIO também sem rádio/LCD. Não se promete corrigir um cartão instável. Dungeon/bolsa ainda não incluídos.

**Entrega anterior: 7.7 — Correção SD e download, 2026.10.13-ota77 / build2026101302.** [Release](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.13-ota77) · [Evidências](docs/OTA_ARTES_CONFIABILIDADE.txt). Inicialização SD antes do LCD e diagnóstico específico; fonte/tag `93b1b4ed`. Reparo completo e jogo confirmados pelo proprietário com SanDisk64GB; falhas persistiram no cartão novo4GB.

**Entrega anterior: Pacote 7.7 — Heróis e inimigos autorais, 2026.10.12-visual77 / build 2026101208.** [Release](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.12-visual77) · [Relatório e limites](docs/PACOTE_7_7_RELEASE.txt) · [Qualificação final](https://github.com/Oflodorporto/RPG-POKET-2/actions/runs/38313621773) · [Publicação e downloads](https://github.com/Oflodorporto/RPG-POKET-2/actions/runs/38316390948). Fonte/tag `7d946aca`; main integrada. 29 inimigos originais, 116 poses/56 efeitos compactos, coreografias por ação e mãos/armas existentes ampliadas; 428 artes não inimigas, Save28 e regras preservados. 63 suítes/22 alvos ASan/UBSan, compilação, manifesto, transição simulada 7.6→7.7 e downloads reais aprovados. Firmware 2.677.248 bytes, margem OTA 468.480. **Teste físico 7.7 pendente.** Novos cenários de cidades/acampamento e redesenho completo de mãos/antebraços ficam para etapas posteriores.

As fatias preparatórias 7.7 permanecem documentadas: [apresentação](docs/PACOTE_7_7_APRESENTACAO.txt), [candidata combinada](docs/PACOTE_7_7_COMBINADO.txt) e [medição de oclusão](docs/PACOTE_7_7_COMBINADO_VALIDACAO.json). Seus hashes e tamanhos -dev são históricos; a Release final usa os dados acima.

**Entrega anterior: Pacote 7.6 — Fadiga e descanso, 2026.10.12-fatigue76 / build 2026101203.** [Release](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.12-fatigue76) · [Regras, transação auxiliar e limites](docs/PACOTE_7_6_FADIGA_DESCANSO_PLANO.txt) · [CI final sem publicação](https://github.com/Oflodorporto/RPG-POKET-2/actions/runs/38291561694) · [CI da publicação](https://github.com/Oflodorporto/RPG-POKET-2/actions/runs/38292646904). 55 suítes, 14 alvos com sanitizadores, compilação ESP32, manifesto e downloads SHA-256 verificados no código `26a4314`. Save28 e 488 artes preservados. Descanso gratuito no abrigo; sem punição offline. **Teste físico rápido 7.6 confirmado pelo usuário em 12/10/2026**: relatou que está funcionando, sem detalhar cenários. Ensaios detalhados/prolongados, falhas e memória em execução continuam pendentes. A publicação havia sido autorizada sem teste prévio. Auxiliar incerto zera somente fadiga; downgrade com exclusão/recriação idêntica não é garantido. [Continuidade atual](docs/CONTINUE_ATUAL.txt).

**Entrega anterior: Pacote 7.5 — Comércio regional, 2026.10.12-commerce75.** [Release](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.12-commerce75) · [Regras e validação](docs/PACOTE_7_5_COMERCIO.txt) · [Economia](docs/PACOTE_7_5_ECONOMIA.csv). Venda com quantidade e confirmação, proteção de itens, preços sem lucro artificial e lojas por região. 50 suítes/nove sanitizadores/compilação e arquivos publicados verificados na entrega. Testes físicos 7.5 aprovados pelo usuário em 12/10/2026.

**Entrega anterior: Pacote 7.4 — Progressão regional, 2026.10.11-balance74.** [Release](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.11-balance74) · [Regras e validação](docs/PACOTE_7_4_PROGRESSAO.txt) · [Medições antes/depois](docs/PACOTE_7_4_MEDICOES.json) · [CI](https://github.com/Oflodorporto/RPG-POKET-2/actions/runs/38114649102). 48 suítes, sete alvos com sanitizadores, compilação real e downloads verificados. Save28, curva D&D, slots, OTA e 488 artes preservados. Os seis testes físicos de 7.4 foram aprovados pelo usuário antes de 7.5.

**Entrega anterior: Pacote 7.2 — Primeira experiência, 2026.10.11-experience72.** [Release](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.11-experience72) · [Relatório 7.2](docs/PACOTE_7_2_PRIMEIRA_EXPERIENCIA.txt). Os seis testes físicos desta versão foram aprovados pelo usuário. Save28 e 488 artes preservados.

**Pacote 7.3 — Auditoria de sessões prolongadas concluída em software.** [Relatório](docs/PACOTE_7_3_ESTABILIDADE.txt) · [Evidências](docs/PACOTE_7_3_EVIDENCIAS.json) · [Validação](https://github.com/Oflodorporto/RPG-POKET-2/actions/runs/38111881866). 47 suítes, seis alvos com sanitizadores e compilação Waveshare aprovados. Nenhuma falha nova de firmware comprovada; acrescentados testes e corrigida documentação. Nenhuma Release exclusiva de 7.3. Testes prolongados físicos e medição de memória/latência na placa ainda pendentes. As entregas posteriores 7.4 e 7.5 estão descritas acima.

Os registros anteriores abaixo descrevem cada entrega no momento em que foi feita; pendências históricas não substituem o estado atual acima.

**Entrega anterior: Pacote 7.1 — Auditoria de Confiabilidade, 2026.10.10-audit71.** [Release](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.10-audit71) · [Relatório](docs/PACOTE_7_1_AUDITORIA.txt). Recuperação NVS, compatibilidade OTA e artes de reserva; aprovado na placa pelo usuário.

**Entrega anterior: Pacote6 — Oito Caminhos de Aeldra,2026.10.10-eventos8.** [Release](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.10-eventos8) · [Guia](docs/PACOTE_6_OITO_CAMINHOS.txt). Oito tipos de cartas, decisões e expedição em primeira pessoa.42suítes, compilação e downloads verificados; save28 lê1..28,488artes preservadas. Teste físico desta versão pendente; cartas3 aprovada pelo usuário. Próximo:Pacote 7.

**Entrega anterior: Pacote6.3 — Vidas na Estrada,2026.10.10-cartas3.** [Release](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.10-cartas3) · [Guia](docs/PACOTE_6_3_VIDAS_NA_ESTRADA.md). Coleta real e escolta comD20/carroça/viagem7s, três tipos diários distintos e duas oportunidades.40suítes, compilação e downloads verificados; teste físico pendente. Save27 lê1..27;488artes preservadas. Faltam cinco dos oito tipos do Pacote6.

**Entrega anterior: Pacote6.2 — Cartas ao Entardecer,2026.10.10-cartas2.** [Release](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.10-cartas2) · [Guia](docs/PACOTE_6_2_CARTAS_AO_ENTARDECER.md). Duas oportunidades por dia, cartas pendentes preservadas, vitória comXP/ouro legíveis no pergaminho.39suítes, compilação e downloads verificados; teste físico pendente. Save26 lê1..26;488artes iguais a mundovivo1. Oito tipos de eventos ainda faltam.

**Entrega anterior: Pacote6 — Caminhos Vivos,2026.10.10-mundovivo1.** [Release](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.10-mundovivo1) · [Guia](docs/PACOTE_6_CAMINHOS_VIVOS.md). Mares/Aurora com3inimigos cada,12contratos porcidade decombate/recuperação/escolta real;39suítes/compilação/downloadsverificados;teste físico pendente. Save25 lê1..25;488artes. Próximo:8eventos/2ofertas diárias doPacote6, ainda nãoimplementados.

**Entrega anterior: Pacote5 — Vozes de Aeldra,2026.10.09-vozes1.** [Release](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.09-vozes1) · [Guia](docs/PACOTE_5_VOZES_DE_AELDRA.md).12 NPCs, falas contínuas conforme progresso e pergaminho animado com retratos/cenários locais.35suítes, compilação e downloads conferidos; teste físico pendente. Save21 lê1–21; artes459 mantidas. Pacote5 finais e Pacote6 variedade continuam planejados.

**Entrega anterior: Pacote5 — Aurora: Arquivos da Coroa,2026.10.09-aurora1.** [Release](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.09-aurora1) · [Guia](docs/PACOTE_5_AURORA_ARQUIVOS_DA_COROA.md). Liora/Nv18 após Mares, missão da Sentinela/seis cenas com retratos/diário.34suítes, compilação e downloads conferidos; teste físico pendente. Save21 lê1–21; leitor anterior não abre21. Artes mantidas. Contribuições/confrontos/finais seguem próximos.

**Última entrega planejada do Pacote4:4.6 — Bestiário do herói,2026.10.09-bestiario1.** [Release](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.09-bestiario1) · [Guia](docs/PACOTE_4_6_BESTIARIO_DO_HEROI.md). Menu > Bestiário: criaturas encontradas por personagem.33suítes/compilação/downloads conferidos; teste físico pendente. Save20 lê1–20; firmware anterior não abre save20. Arte mantida. Próximo: Pacote5 Aurora.

**Entrega anterior: Pacote4.5 — Conheça o inimigo,2026.10.09-inimigos1.** [Release](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.09-inimigos1) · [Guia](docs/PACOTE_4_5_CONHECA_O_INIMIGO.md). Ficha do adversário atual/intenção/tipo/dicas/limite de dano, sem gastar turno.32suítes/compilação/downloads conferidos; teste físico pendente. Save19/artes mantidos.

**Entrega anterior: Pacote4.4 — Fúria ancestral,2026.10.09-barbaro1.** [Release](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.09-barbaro1) · [Guia](docs/PACOTE_4_4_FURIA_ANCESTRAL.md). Bárbaro: Crítico brutal com1/2/3d6 nos níveis9/13/17 e Fúria persistente15.31suítes/compilação/downloads conferidos; teste físico pendente. Save19/artes mantidos.

**Entrega anterior: Pacote4.3 — Domínio arcano,2026.10.09-arcano1.** [Release](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.09-arcano1) · [Guia](docs/PACOTE_4_3_DOMINIO_ARCANO.md). Mago: bônus de INT10 e Mísseis/Raios gratuitos18, fichas passivas/custos reais.30suítes/compilação/downloads conferidos; teste físico pendente. Save19/artes mantidos.

**Entrega anterior: Pacote4.2 — Trilha do Campeão,2026.10.09-campeao1.** [Release](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.09-campeao1) · [Guia](docs/PACOTE_4_2_CAMPEAO.md). Guerreiro: crítico3/15 e Sobrevivente18, fichas passivas sem custo.29suítes/compilação/downloads conferidos; teste físico pendente. Save19 e artes mantidos.

**Entrega anterior: Pacote4.1 — Paladino,2026.10.08-paladino1.** [Release](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.08-paladino1) · [Guia](docs/PACOTE_4_1_PALADINO.md). Punição divina2/aprimorada11 e corte de luz.28suítes/compilação/downloads conferidos; teste físico pendente. Save19 e artes mantidos.

**Entrega anterior: Pacote4 — Progressão recompensadora, 2026.10.08-progressao1.** [Release](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.08-progressao1) · [Guia](docs/PACOTE_4_PROGRESSAO_RECOMPENSADORA.md) · [Continuidade](docs/CONTINUE_PROGRESSAO1.txt). Trilha dos ganhos reais1–20, próximo marco/XP, prévia e confirmação de atributos/talento. As27 suítes, compilação e downloads foram verificados; teste físico aguarda o usuário. Save19/artes SD inalterados. Medição40.960 combates; novos poderes avançados/bestiário/balanceamento econômico ainda pendentes.

**Entrega anterior: Pacote3 — Trancas e gazuas, 2026.10.08-trancas1.** [Release](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.08-trancas1) · [Guia](docs/PACOTE_3_TRANCAS_E_GAZUAS.md) · [Continuidade](docs/CONTINUE_TRANCAS1.txt). Força/Destreza, ferramenta consumível, risco explícito e tentativas persistidas; Save19/leitura1–19. As26 suítes, compilação e downloads foram verificados; teste físico aguarda o usuário. Arte SD inalterada. Próximo: Pacote4 — Progressão recompensadora.

**Entrega anterior: Pacote2 — Exploração variada, 2026.10.08-exploracao1.** [Release](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.08-exploracao1) · [Guia e teste físico](docs/PACOTE_2_EXPLORACAO_VARIADA.md) · [Continuidade](docs/CONTINUE_EXPLORACAO1.txt). Achados, baús, mímicos e sucata; Save18 com leitura1–18. As25 suítes, compilação e downloads finais foram verificados; teste físico desta entrega aguarda o usuário. O pacote de artes permanece igual ao do Pacote1. As versões abaixo são histórico.

2026.10.07-lore1: primeira integracao de As Cinzas da Primeira Aurora, abertura com Nara e carta de Maelis, Diario por progresso existente, Pessoas com dialogos locais e atlas de Valdaria. [Uso e limites](docs/REORGANIZACAO_AELDRA.md). Save10/96bytes e artes SD permanecem iguais; a leitura da historia nao altera recompensas ou personagens. Novas regioes, eventos diarios e campanha completa serao etapas posteriores. [Lore](docs/LORE_CANONICA_RPG_POKET_2.md) e [continente](docs/CONTINENTE_VALDARIA.md).

O projeto inclui GFX de terceiros em `src/GFX`; os avisos/licenças existentes acompanham a biblioteca. Nenhuma licença nova é atribuída ao projeto ou às imagens de terceiros sem decisão do proprietário.

## Estradas e cidades

Versao 2026.10.06-estradas1: cidades com niveis e comercio proprios, quatro inimigos originais, exploracao regional e D20 persistente na viagem. Download com retomada e novas tentativas automaticas. [Regras, saves e teste na placa](docs/ESTRADAS_E_CIDADES.md).

A ota1 foi confirmada funcionando na placa pelo usuario. Esta entrega nova exige validacao fisica separada. Saves escritos no formato8; nao voltar ao firmware antigo para continuar estes saves. Sem gravacao automatica ou arquivos .rpg.

## Wi-Fi e relogio

2026.10.06-wifi1: cinco redes salvas, reconexao, indicador RSSI em quatro barras, relogio preto apos um minuto com brilho reduzido, Continuar download e diagnostico HTTPS do GitHub. [Uso e limites do teste](docs/WIFI_RELOGIO_E_DIAGNOSTICO.md).

Firmware e artes mantem saves8 e o pacote estradas1 de417 recursos. Sem gravacao automatica ou .rpg. Testes fisicos de wifi1 confirmados pelo usuario.

## Cripta do Arconte

2026.10.06-dungeon1c: primeira pessoa, dois andares, selo, sete encontros e chefe, cristal por drop/300 ouro, 44 assets originais e saves9 com importacao1..8. [Controles e detalhes](docs/DUNGEON_PRIMEIRA_PESSOA.md). A arte da dungeon acompanha o firmware; artes.pak permanece compativel. Testes fisicos de wifi1 confirmados pelo usuario; dungeon1c confirmada funcionando na placa pelo usuario.

2026.10.07-dia1: hora local e paletas do mundo: [guia dia1](docs/HORA_E_DIA_NOITE.md). Eventos e novos inimigos noturnos continuam planejados.

2026.10.07-cartas1: [carta de Maelis e missao do hipogrifo](docs/CARTAS_E_HIPOGRIFO.md), Save11/128bytes com leitura1–10. Piloto: uma oferta diaria; demais eventos continuam planejados.

## Tela de titulo e jornada

A versao titulo1 abre no titulo em todo reinicio, protege os tres slots na criacao e concentra as configuracoes. Menu > Diario > Objetivo orienta a historia sem modificar os saves. Consulte [guia e testes](docs/TELA_TITULO_E_JORNADA.md).

## Telas do conceito

Abrigo, menu, mapa de Aeldra e Ruinas de Vespera usam as artes fornecidas, com botoes interativos e dados reais. D20 e viagem animados preservados. [Guia e teste na placa](docs/ABRIGO_E_MENU_CONCEITO.md). Versao 2026.10.07-abrigo1.

## Combate, bolsa e acampamento com os conceitos

2026.10.07-paineis1b: seis conceitos adaptados para oito telas, com dados e herois reais. Imagens 240x320 carregadas do SD para PSRAM na inicializacao; sem leitura por quadro. [Guia e teste na placa](docs/NOVAS_TELAS_CONCEITO.md). Atualize firmware e artes por Configuracoes > Atualizacao.

## Evolução e direção de lançamento

2026.10.08-direcao1: novos heróis no nível 1, curva D&D 1–20, atributos e talento Resistente, orientação do objetivo, combate/bolsa/recompensas mais claros. [Evolução e limites da adaptação](docs/EVOLUCAO_DND.md) e [avaliação crítica de lançamento](docs/AUDITORIA_LANCAMENTO.md). Saves anteriores preservados; Save12 não permite downgrade para firmware antigo.

## Grimório e Devoção

2026.10.08-poderes1: cinco magias de Mago, Impor as mãos e Juramento da Devoção com dois poderes e uso compartilhado. [Controles, custos, saves e limites da adaptação](docs/PODERES_DAS_CLASSES.md). Saves13, leitura1–13; heróis antigos preservados. Artes inalteradas.

## Guerreiro e Bárbaro

2026.10.08-martiais1: Segundo Fôlego, Surto de Ação e Fúria com usos por descanso, duração e retomada persistida. [Regras, controles, adaptações e teste na placa](docs/PODERES_MARCIAIS.md). 21 suítes; Save14/leitura1–14; artes inalteradas.

## Revisão para lançamento

2026.10.08-lancamento1 aplica cinco de dez melhorias: intenções de inimigos, preparação de viagem, compra comparativa, recuperação após derrota e guia. [Revisão completa e limites](docs/REVISAO_LANCAMENTO_10_MELHORIAS.md). 22 suítes; save15/leitura1–15; artes compatíveis. A campanha completa ainda está em desenvolvimento.

## A campanha chega a Mares

2026.10.08-mares1: duas missões de Sabela depois da Cripta, nível10+, provas da Mão de Cinza e resgate dos refugiados. [Como jogar e limites](docs/CAMPANHA_MARES.md). 23 suítes, save16/importação1–15, mesmas artes. Aurora e o final ainda estão em desenvolvimento.

## Pacote 1 — Origem e rostos

2026.10.08-origens1: origem por classe, chegada a Carvalho, rostos dos NPCs e primeiro objetivo concreto. [Conteúdo, compatibilidade e testes na placa](docs/PACOTE_1_ORIGEM_E_ROSTOS.md). [Próximos pacotes](docs/ROADMAP_PACOTES_EXPERIENCIA.md).

## Histórico — Pacote 7.2: Primeira Experiência

[2026.10.11-experience72](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.11-experience72): novos personagens chegam diretamente a Carvalho; história pode ser pulada, dicas contextuais são opcionais e o Guia continua disponível. Personagens existentes não são obrigados a repetir as orientações. Dez contextos reutilizam retrato, pergaminhos e cenários existentes. Save28 e as488artes preservados.

45 suítes, verificações de memória e compilação Waveshare aprovadas na entrega de 7.2. Os seis testes físicos de 7.2 foram aprovados pelo usuário. [Relatório histórico](docs/PACOTE_7_2_PRIMEIRA_EXPERIENCIA.txt). Para a versão vigente e próximos passos, consultar o Estado atual e a continuidade acima.

