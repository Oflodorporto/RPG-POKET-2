# Pacote 11: comércio em lote e dez rações

## Estado e autorização

Candidato em `feat/package11-release`, sobre o main exato `6a23dda54114db611b818b51e4b1b942019098fa`. Preparar e qualificar o pacote não autoriza main, tag, release, USB ou OTA física. O workflow desta branch tem permissão apenas de leitura. O publisher futuro só aceita push em main do SHA exato já qualificado nesta branch e exige autorização específica antes de qualquer avanço de main.

Identidade futura: `2026.10.14-shop77`, build `2026101405`, pacote `7.7 - Comercio em lote`. Nunca reutilizar a identidade publicada pvp77/build2026101404 para este código: isso impediria um upgrade OTA verdadeiro a partir da versão PvP atual.

O inventário congela os 992 arquivos de main mais o patch local revisado `21ab532f3bfceb37c2def05c9d2ee1d247df30f9` (árvore original `161c2909dab80fc4d76d8f2c6bad93dcfc1c6ad0`) e a correção de tipos detalhada abaixo. A árvore de funcionalidade corrigida é `4c9e5360b6ed849fec9fdae74ef9580180aa7881`. O arquivo original de entrega tem SHA-256 `d62a0b5bb698694c58cf36abf8b60231bbe71fcbd8769ff1581949cb7a861883`. As exceções explícitas são somente os arquivos de embalagem/gates11, a identidade futura e o publisher futuro. Não alterar os verificadores, inventários e fixtures históricos para fazê-los aceitar o novo pacote.

## Mudanças e preservação

- Seletor de quantidade com total, capacidade e saldo, aplicado na compra e venda.
- Transação em lote inteira, incluindo teste de gravação com falhas, cancelamento, toque repetido e faixa limitada.
- Rações até 10; os demais limites, preços e regras de premiação permanecem iguais.
- A exceção do kit para recuperação de escolta continua válida.
- O pergaminho comercial fica visível no primeiro frame, evitando confirmação invisível.
- Mesmo Save30 de 160 bytes, leitura29/30 e epoch1. Sem reset novo, campo persistente novo ou mudança no contrato USB/OTA.
- PvP v5/52 bytes, história, relógio, combate, geração procedural, Wi-Fi, SD, artes e fontes históricas preservados. Coop, pets e bolsa visual nova ficam fora.
- README atual com apresentação, mídia, histórico e lore preservado byte a byte. Os documentos anteriores mantêm seu contexto histórico; este é o documento vigente do novo candidato.

## Correção de tipos após a primeira compilação ESP32

O run `38607447395`, no SHA `6797d22c75903b7d0cb376b2547b1c097b5d64b0`, passou o agregado completo de 149 tarefas e falhou na compilação real. No core ESP32, `uint32_t` é `unsigned long`, enquanto a quantidade retornada por `tradeStock` é `unsigned int`; a dedução automática de `std::min` não aceitou essa combinação.

A correção especifica `std::min<uint32_t>` somente em `saleMaximum`, sem mudar subtração, divisão, limites, preço, regras, save ou layout. Os valores são limitados pelo guarda de ouro e pelo estoque. O inventário mantém a proveniência do patch original e registra separadamente o SHA do ajuste de uma linha, seus hashes antes/depois e a nova árvore congelada. Os scripts e fixtures históricos não foram alterados.

A configuração agora compila a função exata extraída do fonte em uma verificação de tipos nativa de 32 bits, com `unsigned int` e `unsigned long` distintos, e exige que a expressão anterior falhe. Essa prova é somente de tipos e não substitui o build ESP32. Toda a qualificação de 149 tarefas, compilação real e OTA dos bytes finais deve rodar novamente no novo SHA. A passagem host do candidato anterior não qualifica a correção.

## Compatibilidade do save

A versão nova aceita registros históricos com até 9 e novos com 10. Compartilhar número de formato não significa que o domínio aceito pelos leitores antigos aumentou. Os codecs históricos rejeitam10; um journal misto pode voltar ao registro anterior com até 9, e dois registros com 10 bloqueiam o leitor antigo sem escrita nem limpeza. Os testes cobrem leitura de fontes históricas exatas e o comportamento do journal. Não há garantia de downgrade de firmware.

O manifesto mantém schema 2 e o marcador OTA legado save_format 7, com writes_save_format 30, reads_save_formats 29/30 e epoch1. Metadados adicionais documentam o limite 10, o limite histórico 9 e ausência de suporte a downgrade. O reset histórico já concluído não é repetido.

## Gates obrigatórios

1. Inventário completo com SHA-256 e modos dos arquivos, rejeitando fonte extra, symlinks, arquivo ignorado no sketch, outputs rastreados e alteração de qualquer fonte congelada. Arte reconstruída idêntica: 666 assets, 7.470.928 bytes, SHA-256 `14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56`.
2. Plano agregado de 149 tarefas: as 143 herdadas do pacote10 mais quatro alvos de comércio/rações em otimizado e ASan/UBSan, e dois modos do leitor antigo. A preparação herdada permanece incluída uma vez. LeakSanitizer=1 é obrigatório no CI; resultados locais sob ptrace não substituem esse gate.
3. Toda a cobertura histórica de narrativa, relógio, PvP, save, classes, campanha, procedural, render e falhas de journal continua presente. Capturas reais de comércio se somam às prévias herdadas. As imagens são pixels do renderer host, não medida de FPS, latência ou legibilidade física.
4. Build real ESP32-S3 com core 3.3.12, PSRAM OPI, 16 MiB flash, partição app3M_fat9M_16MB e EraseFlash=none. `--build-path build/esp32-package11 --export-binaries=false`, sem `--output-dir` e sem exportação no sketch. Conferir SHA e inventário após compilar.
5. Verificador do manifesto e bytes reais, testes negativos, adapters OTA com SHA256 real em otimizado e ASan/UBSan/LSan. Incluir predecessor público PvP com hashes dos downloads da publicação, não os hashes diferentes do build de qualificação.
6. Provas de migração Save29 e conservação Save30 em três slots com codecs históricos exatos. Teste separado do limite 10 e rejeição/fallback do leitor antigo. Sem alegação de downgrade ou teste físico.
7. Registrar SHA/tree, inventário completo, todos os checkpoints/logs, versões de compilador, artefatos, captura e orçamento. Guardar evidência como artefato CI, sem publicar release.
8. Publisher futuro reexecuta o agregado, compila novamente e verifica os bytes finais. Não promove cegamente o artefato da qualificação. O gate exige mesmo repositório, SHA, branch, workflow, evento push, status completed e conclusão success. Alterar fontes depois exige novo SHA e qualificação integral.

## Limites da evidência

O ambiente local não dispõe de arduino-cli/core ESP32; nenhuma instalação massiva foi feita. Os testes focados de implementação passaram 26 alvos otimizados e 26 ASan/UBSan, com revisão independente. Isso não é passagem automática do novo agregado 149 ou da compilação ESP32. O estado real deve ser lido no run do SHA e em `package11-source-identity.json`; este documento não declara CI passado antecipadamente.

Não há novo atlas, payload de arte ou chamada de alocação de produção. O orçamento conhecido permanece 7.638.272 bytes de PSRAM, estimativa estática que não mede heap livre, stack, toque ou rádio. O firmware deve caber na partição OTA 3 MiB, conferido nos bytes reais. Testes físicos de compra/venda e toque, leitura dos três slots, atualização no aparelho, SD, Wi-Fi/ESP-NOW e cortes reais de energia continuam pendentes. Aprovações físicas anteriores de história/relógio não qualificam automaticamente este pacote.
