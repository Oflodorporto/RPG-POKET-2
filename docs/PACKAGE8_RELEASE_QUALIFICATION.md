# Pacote 8: Ecos de Valdaria, candidato de qualificação

Identidade: `2026.10.14-story77`, build `2026101402`, geração pública 7.7.
A branch `feat/package8-release` qualifica e guarda artefatos. Este documento e o
workflow preparado não autorizam main, tag, release, flash ou OTA no aparelho.
Publicação exige aprovação separada do usuário depois da qualificação.

## Fonte e escopo aprovados

- Base remota main: `979822bb48ad10180d182df2232e2dcded4f9d66`, árvore `a1e76917fd848b5f2dbc77ad81b18a34c7654257`.
- Essa base difere do pacote 7 publicado `e775b5d1854174b45404ca0cd3305c1e24610861` somente pelo README, preservado.
- Narrativa revisada: `4f4eb428ad48914e2cf987ad8983e11cf0dd5d38`, árvore `372133e00714548c1081fdee98fdbdad9eb8b6b7`.
- Árvore narrativa mais README atual antes do empacotamento: `6419dba3e5430d8986cef26ab023c64f40a73860`.
- Os 893 arquivos desse snapshot são congelados em `tests/fixtures/package8-approved-sources.json`. Só 20 caminhos explícitos de empacotamento são exceções; a única fonte de firmware liberada para empacotamento é `ReleaseVersion.h`.
- Textos e controladores narrativos aprovados permanecem exatos. Save30, leitura Save29/30, epoch1, mecânicas, RNG, custos, PvP e as 666 artes são preservados. Nenhum segundo mapa, nova missão, viagem, recompensa ou flag é implementado.

A revisão cobre encerramento pleno e temporário, reações individuais, origens,
cartas e eventos, descrição de itens, objetivos, primeira/novas expedições e a
carta de Sabela que apresenta Neris e Talassar sem diminuir a vitória em Aeldra.

## Gates obrigatórios

1. Inventário completo, modos e hashes, contra injeção de arquivo ignorado ou link simbólico na árvore de firmware.
2. Plano agregado com 130 tarefas: as 127 herdadas, sem perda de cobertura, mais controlador/eventos/renderer narrativos com ASan, UBSan e LSan (`detect_leaks=1`).
3. Capturas reais 240×320 com fonte e artes de produção; 3.481 frames, 671 páginas PNG únicas, 43.123 verificações de rótulos, além de previews herdados. Capturas não equivalem à inspeção humana de cada frame.
4. ESP32-S3, core 3.3.12, 16 MB, partições `app3M_fat9M_16MB`, PSRAM OPI e sem apagar flash. O log registra a versão real do Arduino CLI.
5. Build fora do sketch, `--build-path build/esp32-package8 --export-binaries=false`, cópia apenas de `RPG_POKET_2.ino.bin` e novo guard de inventário depois de compilar. Nunca usar `--output-dir` nem permitir pasta gerada dentro do firmware.
6. Firmware real compilado: identidade, chip, margem OTA, hashes, manifesto, CRC e pacote de artes. Fixture sintética permanece explicitamente distinta.
7. OTA host com bytes reais: cliente touch77 recusa; scenes77/scale77/camp77 migram os três slots Save29 para30; rollback para codecs29 rejeita Save30 sem escrita. Explore77 e immersion77 passam por ida e volta com Save30 e leitura histórica estritamente sem escrita.
8. Registrar commit/árvore exatos, resumo agregado, hashes, tamanhos e orçamento. Fonte limpa antes e depois. O processo de publicação futuro exige sucesso da qualificação no mesmo SHA, reexecuta os gates e valida downloads públicos.

## Compatibilidade e limites

O pacote escreve Save30, lê Save29/30 e não acrescenta reset: epoch1 continua a
marca histórica já concluída. `reset_game_slots_once` no manifesto descreve o
contrato antigo, não um novo apagamento a cada atualização. Gerações anteriores a
Save29 mantêm a orientação USB inicial; nunca usar erase-all.

O pacote de artes permanece exatamente 7.470.928 bytes, SHA-256
`14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56`.
O orçamento conhecido de PSRAM é 7.638.272 bytes (7.470.912 de payload,
153.600 de framebuffer e 13.760 de decode); não é medida de heap livre. Nenhuma
alocação adicional ou atlas novo foi introduzido por esta narrativa. A margem
OTA só pode ser declarada depois do link ESP32.

Não há aparelho físico nesta qualificação. Toque real, FPS, heap em execução,
leitura do cartão e experiência final na placa permanecem sem validação física.
Há uma sobreposição visual herdada na linha de custo sobre o retrato de certas
contribuições; não foi ampliado o escopo para alterá-la. As fixtures de vitória
usam transições reais com HP inimigo zerado para focalizar estado, sem pretender
nova avaliação de balanceamento.

Status deste documento: candidato preparado; CI ESP32/LSan/bytes reais precisa
terminar com sucesso antes de indicar qualificação. Nenhuma publicação implícita.
