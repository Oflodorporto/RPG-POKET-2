# Auditoria Ponytail — RPG POKET 2

## Base e limites

Auditoria por leitura em 19/10/2026, antes de alterar código de produção. Base pública e HEAD remoto confirmados: `aecfabf32edba31fce567a32450503be3492ea66`. Release latest confirmada pela API: `v2026.10.17-wifitest77`, firmware **3.006.736 bytes**, SHA256 `749ebca3cfd13651dbfe709d4c1003aa9860b3cefe7b5e641ad4859b7f4fa1b6`. Esses bytes são os publicados, não uma medição da compilação local.

O checkout `2026-10-17/task-3/ble-prototype` tem mudanças BLE não commitadas. Foi apenas inspecionado e copiado com Git, sem incorporar ou alterar suas mudanças. Trabalho isolado em `audit/ponytail-refresh`, worktree `2026-10-18/task/rpg-ponytail`. Nenhum AGENTS.md ou skill local foi encontrado na árvore do RPG. Nenhum registro privado de sessão foi acessado.

Referência fixada: [Oflodorporto/ponytail @ 09c434ab](https://github.com/Oflodorporto/ponytail/tree/09c434ab73ca43761c5d8b0c5d341926c2fba710). Lidos README, `skills/ponytail/SKILL.md`, `skills/ponytail-audit/SKILL.md` e `skills/ponytail-review/SKILL.md`. Aplicação exclusivamente como metodologia: nenhum instalador, hook, importação de histórico ou telemetria. Percentuais do README não são resultados deste jogo.

## O que o repositório faz

Firmware de RPG em português para Waveshare ESP32-S3-Touch-LCD-2 SKU29667. Um jogador por placa, três slots, campanha por turnos, PvP entre duas placas e treino coop de duas pessoas somente em RAM. Carga considerada: uma thread principal de UI/jogo, callbacks da tarefa Wi-Fi em outro contexto e worker de OTA; LCD e SD compartilham SPI. Não é um serviço para muitos usuários.

## Mapa e fluxo de dados

| Caminho | Entrada → processamento → saída | Evidência e proteção existente |
|---|---|---|
| Inicialização | setup → SPI/SD antes do LCD → framebuffer/artes PSRAM → NVS → título | `RPG_POKET_2.ino:758`, `SharedStorage.h:8`, `FrameBuffer.h:14`; sem formatar SD em falha. Falta de PSRAM das artes bloqueia sem apagar saves. |
| Toque/UI | I2C CST816 → validade/coordenadas → TouchGate → fila de um toque vinculada à página/fase → tapped | `RPG_POKET_2.ino:70-98,809-812`, `TouchGate.h`; liberação válida contínua de 30 ms; toques durante animações não viram ações futuras. |
| Campanha/combate | controller → regras puras/RNG do Game → receipt visual → journal → próxima página | `RPG_POKET_2.ino:111-150`, `Rules.h:276-360`, `CombatTrace.h`; erro de save suspende transição, retry explícito. Progressão, história, preços e dificuldade fora da alteração. |
| Renderer/artes | render → Canvas/GFX → framebuffer 240×320 RGB565 → faixas SPI de 16 linhas | `View.h:100`, `RPG_POKET_2.ino:50-68`; 153.600 bytes PSRAM, sem alocação por frame. Coop gira coordenadas, não o stride físico. |
| Assets | SD → tamanho/cabeçalho/CRC → artMemory ou fallback | `DeviceSettings.h:23-72`, `AssetStore.h`, `AssetCatalog.h`; 666 entradas, payload 7.470.912 bytes. `equipment01`/`effects01` são fontes opcionais, não carregadas automaticamente. |
| Persistência | Game validado → Save30 160 bytes → duas cópias + sequência/CRC → leitura de retorno | `Persistence.h:16-111,124-245`, `NvsPersistence.h`; Missing ≠ erro I/O, futuro bloqueado, migração 29/30, slot/epoch protegidos. Nenhum erase global NVS. |
| PvP | callback → fila fixa → Session → Duel determinístico → reserva/resultado persistido | `ClubRadio.h:15-96`, `ClubProtocol.cpp`, `ClubDuel.cpp`; protocolo v5/52 bytes, prova terminal e política de resultado não confirmado. Ver achado 2 no adaptador de hardware. |
| Coop | callback com geração → fila fixa → Session/admissão → Endpoint → snapshots/ACK → animação | `CoopRadioDevice.h`, `CoopSession.h`, `CoopEndpoint.h`, `CoopWire.h`; pares/fila limitados, heartbeat/retries existentes, replay determinístico, campanha/RNG não escritos. Ver achado 1 na saída. |
| Diagnóstico Wi-Fi | dono exclusivo → consentimento → sondas → RAM limitada → exportação SD explícita | `WifiRadioTestDevice.h:196-287`, `WifiRadioTestRuntime.h`, `WifiRadioTestExport.h`; readback não é potência irradiada. Já conserva posse quando cleanup falha. |
| OTA | HTTPS/CA/domínios permitidos → manifesto limitado/compatibilidade → download retomável SD → SHA/CRC → partição livre → boot | `UpdateDevice.h:19-27,97-124`, `UpdateInstall.cpp:17-73`, `ArtPromotion.h`; validação antes da troca de boot, artes versionadas/backup, mutex SPI e worker. Não alterar partições, codec ou identidade de release nesta etapa. |
| Build/testes | Bash/Python/G++ → testes puros, fakes de SDK, renders, sanitizers → Arduino core 3.3.12 → gates de bytes | `.github/workflows/wifitest77-check.yml:25-72`, `tools/test_native.sh`, `tools/test_wifitest77_local.py`; agregado histórico tem 165 grupos. Gates de fonte fixam identidade antiga e não qualificam automaticamente um novo diff. |

## Deve corrigir

1. **Coop libera o rádio após falha de fechamento/restauração** (`firmware/RPG_POKET_2/Coop/CoopRadioDevice.h:94-107,198-204`; `CoopRuntime.cpp:24,30-37`; `CoopDevice.h:10-16`).
   - **O que faz:** fecha ESP-NOW, restaura o Wi-Fi anterior e retorna à Guilda.
   - **Problema/evidência:** `esp_now_deinit()` falha, mas `radioReady=false` e `release(Training)` ainda executam. Se restaurar canal/reconexão/storage falhar, `wifiTouched=false` elimina a possibilidade de retry e a UI sai mesmo assim. Uma nova sessão pode usar um rádio que não terminou a anterior.
   - **Correção mínima:** guardar etapas ainda pendentes, conservar lease e tela exclusiva, impedir novas ações de sessão e permitir repetir a limpeza. Reusar o padrão já existente em `WifiRadioTestDevice.h:274-285`, sem transplantar toda a stack.
   - **Se adiar:** falhas do driver deixam posse inconsistente e o jogador perde a opção de restaurar. Isso não explica nem corrige alcance físico.
   - **Teste:** injetar falha em unregister/deinit, canal, reconnect e storage; verificar retenção da posse, callback cercado, retry idempotente e bytes do save/RNG inalterados; exercitar saída e falha durante open pelos chamadores reais.

2. **PvP informa abertura bem-sucedida quando APIs essenciais falham** (`firmware/RPG_POKET_2/ClubRadio.h:32-38`).
   - **O que faz:** prepara STA/canal, cria fila, registra callback e obtém MAC antes de admitir partidas.
   - **Problema/evidência:** apenas `esp_now_init` é checado. Falha de `esp_now_register_recv_cb` deixa `arena.opened=true` e retorna true sem receber; falha de `esp_wifi_get_mac` usa bytes locais não inicializados; fila nula também não bloqueia. São caminhos de falha de API, não uma afirmação de reprodução em hardware.
   - **Correção mínima:** verificar cada pré-requisito antes de publicar `opened`, MAC inicialmente zerado, rollback claro e nenhum pagamento/reserva antes de abrir. Falhas de teardown também precisam manter posse, seguindo o achado 1; rever ambos os chamadores de `closeClub`.
   - **Se adiar:** arena sem transporte funcional, identidade indefinida e diagnóstico enganoso. Não altera cálculo de dano nem protocolo.
   - **Teste:** fake SDK com falhas independentes de fila, mode/channel/MAC/callback; confirmar false, nenhuma cobrança e retorno seguro. A revisão integral do ciclo PvP é etapa separada se não couber com validação adequada.

## Deveria corrigir

3. **Documentação dirige o jogador à release anterior** (`README.md:10,18,101-102`; `docs/ESTADO_ATUAL.md:7,32`).
   - **O que faz:** links e estado apresentam coopdiag77 como atual.
   - **Problema/evidência:** latest e `ReleaseVersion.h` são wifitest77, confirmados no início desta auditoria. A seção de publicação antiga afirma que latest aponta para coopdiag77.
   - **Correção mínima:** atualizar a identidade e links atuais e marcar a evidência de 15/10 como histórica; preservar seus limites e não atribuir a ela validação do novo diff local.
   - **Se adiar:** o jogador instala o diagnóstico anterior e compara placas com versões diferentes.
   - **Teste:** conferir tag/build/bytes contra API e header; links atuais devem concordar.

4. **Testes do adaptador coop não injetam falha na saída** (`tests/coop_training_radio.cpp:35-38`; `tests/coop_fake/esp_now.h:9-17`; `tests/coop_training_device.cpp:23-26`).
   - **O que faz:** cobre abertura, rollback inicial, duas threads, filas e reentrada.
   - **Problema/evidência:** deinit/unregister/reconnect do fake sempre têm sucesso. A regressão do achado 1 passa invisível.
   - **Correção mínima:** estender o fake existente e testar saída no rádio e no bridge real; conservar testes de concorrência.
   - **Se adiar:** fechamento inseguro continua com testes verdes.
   - **Teste:** o novo cenário deve falhar na base, passar na correção e também verificar retry repetido/sem NVS.

## Melhoria pequena

5. **Equipamentos de níveis diferentes ficam visualmente iguais na grade** (`firmware/RPG_POKET_2/BagView.h:18-21`).
   - **O que faz:** usa um ícone por família e imprime quantidade fixa 1; o nome só aparece na seleção.
   - **Problema/evidência:** três cajados/espadas/etc. na mesma página usam a mesma imagem e o mesmo 1; o tier/bonus só é distinguível pelo nome após selecionar.
   - **Correção mínima:** mostrar tier e bônus reais na grade e, se o build confirmar margem, reaproveitar os seis PNGs 32×32 de equipamento de `assets/reusable/equipment01`; nenhuma regra, slot, preço ou item novo. Conversão RGB565 com máscara alpha, sem alterar artes.pak/PSRAM.
   - **Se adiar:** mais toques para comparar inventário; não é defeito de combate.
   - **Teste:** renders das 18 peças/seleções/páginas, equipado e guardado, labels dentro da tela, alpha/preto preservados e Game/save inalterados. Comparar binário real com partição de 3.145.728 bytes.

## Achado adicional durante a validação Windows

6. **Checkout Windows altera bytes de fontes com hashes fixos** (`assets/enemies-original1/source/CHECKSUMS.json`; `tools/prepare_enemy_art.py:57-64`).
   - **O que faz:** a reconstrução compara tamanho e SHA dos arquivos-fonte antes de montar o pacote.
   - **Problema/evidência:** com `core.autocrlf=true`, o checkout converteu LF em CRLF. `tools/unpack_art.py` interrompeu com `source/CHECKSUMS.json: byte length mismatch`; 115 arquivos em assets diferiam dos blobs Git, sem mudanças semânticas intencionais.
   - **Correção mínima:** `.gitattributes` preserva bytes exatos em `assets/**` e `cartao/**` com `-text`. Fontes restauradas dos blobs imutáveis; nenhum validador ou hash foi relaxado.
   - **Se adiar:** o Windows não reconstrói as artes verificadas e sugere corrupção inexistente. O teste é um checkout isolado com autocrlf=true seguido de comparação com o blob; o pacote reconstruído também deve manter o SHA público.

## Aplicação da metodologia

| Regra Ponytail | Aplicação nesta tarefa |
|---|---|
| Map first; entender carga e fronteiras | Mapa acima, callbacks vs loop, SPI compartilhado, worker OTA, leitura de persistência antes do diff. |
| Correct → safe → load → tested → fast → lean | Primeiro erros de posse/driver; não cortar validação, resets protegidos ou a qualificação por economia de linhas. |
| Reusar antes de criar | Manter Game/Rules, renderer/GFX, journal, filas e protocolos; padrão de cleanup do diagnóstico já disponível. |
| Menor mudança completa | Corrigir o ciclo atingido e os chamadores; não reescrever motor, bootstrap, rádio ou todo roadmap. |
| Lógica deixa teste | Falha de teardown primeiro reproduzida; testar retry e UI, além do sucesso. |
| Review além do diff | Rever CoopRuntime/CoopDevice, entrada pela Guilda, loop exclusivo, proprietário comum e coexistência com PvP/diagnóstico/OTA. |
| Dizer o não verificado | Sem teste físico RF/toque/OTA/FPS/heap; build e testes locais registrados separadamente. |

**Veredito da auditoria:** corrigir o achado 1 com regressão primeiro; tratar 2 sem esconder falhas ou mudar regras. O projeto tem proteções úteis para reaproveitar. Uma reescrita total aumentaria a superfície de risco sem prova de benefício.

**Lean:** nenhum número de linhas/dependências economizadas é prometido. Não há evidência que justifique remover as camadas de persistência ou confiabilidade. Os headers de arte gerados não foram auditados número por número; limites, catálogo/CRC e consumidores foram inspecionados. Scripts históricos de preparação e todos os 165 grupos não foram integralmente executados nesta fase de leitura. Compilação local, testes antes/depois e revisão final serão documentados no relatório de entrega.

**Não verificado:** placa real, alcance/antena/alimentação, RF irradiada, toque e orientação físicos, tempo de quadro, heap mínimo, queda de energia/OTA físicos, fontes originais não presentes no repositório. Nenhum push, release, deploy ou flash autorizado ou executado.
