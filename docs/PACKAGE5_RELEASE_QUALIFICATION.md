# Pacote 5: contrato de qualificação

Candidato `2026.10.13-camp77`, build `2026101308`, pacote `7.7 - D20, acampamento e bolsa`. O número público da geração continua 7.7. Este documento define os gates; não é uma declaração de aprovação nem de publicação.

## Fonte e escopo

A camada funcional contém exatamente os 217 arquivos da revisão congelada, registrados em `tests/fixtures/package5-approved-sources.json`. Base funcional scale77: commit `ad8acd8b60f8c4c8bf961f87f8300445ad9d6be5`, árvore `004fbb47be6953c1437d4607bcba31ffab235ba6`. A preparação descende também da documentação publicada em `ae3616038c222939b835aae7c5f0b8eca2206eaf` (somente README).

Inclui D20 geométrico real, personagem personalizado no acampamento comum, tenda no mapa e acesso à Bolsa no hub das Ruínas. As fontes reutilizáveis de equipamentos e efeitos ficam no repositório, sem ampliar o pacote de artes carregado. Não inclui o futuro registro detalhado de rolagens de combate nem geração procedural de dungeon.

`check_package5_scope.py` exige cada hash funcional aprovado, árvore da base, todos os demais arquivos herdados byte a byte e somente as 13 exceções explícitas de empacotamento. Nenhum arquivo ignorado em `build/` é entrada do gate. Os verificadores históricos permanecem inalterados; seus escopos antigos não são enfraquecidos para aceitar esta entrega.

## Persistência, atualização e recursos

- Leitura e gravação Save29 somente; `SAVE_RESET_EPOCH=1`, sem migração ou novo reset.
- `artes.pak`: 666 assets, payload7.470.912, total7.470.928 bytes, CRC32 `9b4bcfc0`, SHA-256 `14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56`.
- PSRAM conhecida: 7.638.272 bytes, incluindo framebuffer153.600 e workspace13.760. Essa conta não mede heap livre.
- Tenda: 1.080 bytes const na flash. D20: arrays principais de pilha168 bytes. Linha do botão Bolsa:176 bytes. Renderização sem novo buffer PSRAM ou alocação de heap; wrappers de alocação permanecem nos testes D20/acampamento.
- O binário real deve ser ESP32-S3, conter a identidade exata e caber em `0x300000`; margem e SHA-256 são registrados após o link.

O adaptador OTA compila closures históricas intactas de touch77 (`91c64a364a7c1c697b9cd91b7e72027fa03f2dee`, árvore `1099a6cb0713f22ed805a8ca28f3978f77734727`), scenes77 (`dd626414b62bf6f88638ffe5d542196230c51061`, árvore `d1d9972abe0e792bd2adbf51f39cd720d354a074`) e scale77 (base acima). Touch77 deve recusar Save29-only; tanto scenes77 quanto scale77 devem aceitar o novo build. Saltos de build são válidos: o alvo deve ser exatamente2026101308 e maior que o cliente, sem exigir incremento unitário. O caso sintético de versão futura usa somente o candidato+1.

Cada caminho histórico preserva byte a byte seus três registros Save29 e marcador Done da época1, com barreira sem escrita/apagamento. Os probes negativos incluem metadados, corrupção, hash, placa, partição, rollback e reparo de artes da mesma versão.

## CI do SHA exato

Branch exclusiva `feat/package5-release`, workflow `package5-check.yml`, permissão somente leitura, checkout de histórico completo. `RPG_DETECT_LEAKS=1`; `test_package5.sh` executa uma vez as 91 suítes otimizadas e 50 com ASan/UBSan herdadas, mais cinco novas em ambos os modos: D20 render, D20 controller, camp identity render, Ruins bag controller e Ruins bag render. Totais previstos:96/55, além dos probes de catálogos/OTA. A opção local `--focused` não é usada pelo CI.

Core ESP32 3.3.12, ESP32-S3/240MHz/16MiB, partição `app3M_fat9M_16MB`, OPI PSRAM, `EraseFlash=none`. O workflow produz os arquivos reais com `release_manifest.py`, verifica-os e roda os adaptadores normais e sanitizados. Modos de firmware sintético são apenas testes locais do harness, nunca substitutos da compilação.

Artefatos retidos: firmware, artes, manifesto, SHA256SUMS, SHA/árvore da fonte, orçamento, logs, D20 animado/todas as faces, mapa com tenda, acampamentos ready/fallback e quatro períodos nas Ruínas. As capturas precisam ser comparadas às prévias congeladas; aprovação visual e limites físicos devem ser explicitados.

## Publicação separada

Não promover `main`, criar tag ou publicar durante a qualificação. Depois de autorização específica, a promoção precisa preservar o mesmo SHA qualificado. Qualquer nova edição ou merge exige nova qualificação. `release.yml` continua main-only, exige sucesso do workflow/branch/SHA exatos, tag inédita, tag apontando ao SHA testado e conferência dos três downloads e do manifesto latest. Teste físico de toque, heap, FPS e estabilidade na placa continua separado e não é inferido do CI.
