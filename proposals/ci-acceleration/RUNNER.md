# Runner completo do benchmark coopdiag77

## Estado e limites

`run_benchmark.py` implementa a preparação e a execução completa de uma perna
fria ou quente. O código e os testes estão nesta proposta isolada. Os testes
locais usam fixtures explícitas e subprocessos simulados: **nenhum agregado de
produto, build ESP32 ou gate OTA foi executado por estes testes**. Este documento
não afirma que o workflow remoto passou.

O runner não publica, não cria release, não envia commits, não modifica os
workflows de qualificação/publicação existentes e não emite autorização de
publicação. Ativar o workflow da proposta e enviá-lo remotamente exige revisão
separada. Os resultados recebem `benchmark_only=true` e `cannot_publish=true`.
O collector ainda precisa verificar a origem Actions, a conclusão dos dois
jobs e os tempos de transferência antes de produzir a comparação.

## Entradas e identidade

A interface é exclusivamente:

```sh
python3 "$ORCHESTRATOR/proposals/ci-acceleration/run_benchmark.py" prepare \
  --source "$SOURCE" --orchestrator "$ORCHESTRATOR" --work "$WORK" --phase cold

python3 "$ORCHESTRATOR/proposals/ci-acceleration/run_benchmark.py" qualify \
  --source "$SOURCE" --orchestrator "$ORCHESTRATOR" --work "$WORK" --phase cold
```

Para a segunda perna, usar `--phase warm` em um **novo job/runner**, com checkouts
e diretórios novos. Os mesmos caminhos textuais em máquinas distintas são
permitidos; reaproveitar os diretórios de uma execução não é permitido.

- Todos os caminhos são absolutos, canônicos, sem symlinks e disjuntos.
- Source deve ser um checkout Git real e não shallow do commit
  `6026fb49db4fe874b9c3f0571cbe4fdb9e66adb3`, árvore
  `5ad62cddfe052bea09bf99c29989731722088981`, filho único de
  `c8738eb67bde3817feaab9f3a53c30607396a5f5`.
- Orchestrator deve ser outro checkout Git real, completo e limpo, em
  `GITHUB_SHA`, distinto da fonte. O próprio runner deve ser executado desse
  checkout. A árvore real, o workflow e seu SHA-256 são registrados.
- O contexto real deve ser push no repositório `Oflodorporto/RPG-POKET-2`, na
  branch exata `bench/coopdiag77-cache-promotion`, usando
  `.github/workflows/ci-acceleration-benchmark.yml` desse mesmo commit.
- Repository ID, SHA, ref, workflow ref/SHA, run ID e attempt são validados pelo
  adapter. Nenhuma variável `GITHUB_*` é reescrita para fingir uma qualificação
  de produção.
- O runner exige `RPG_DETECT_LEAKS=1`, `PYTHONDONTWRITEBYTECODE=1`, Python sem
  otimização, Python 3.12.3, g++ histórico exato já antes de baixar dependências,
  Linux/X64, `ImageOS=ubuntu24` e `ImageVersion`
  exata. Os gates originais dependem de assertions; executar o runner com `-O`
  ou desabilitar assertions por `PYTHONOPTIMIZE` é recusado.
- O inventário agregado, compilador host, plano de comandos, CLI 1.5.2-rc.1,
  core 3.3.12, compilador target esp-x32/2601, Pillow 11.3.0 e PyYAML 6.0.3 são
  conferidos contra os contratos revisados. Uma imagem hospedada que muda o
  compilador/Python exige diagnóstico e nova decisão de baseline, não relaxamento
  silencioso dos pins.

## Sequência de integração

### 1. Prepare

`prepare` usa apenas a biblioteca padrão e os módulos locais revisados. Lê
`source-contract.json`, confere todos os originais pinados e o hash exato de
`reviewed-plan.json`. A receita literal e os padrões de artefatos são lidos do
workflow original **após verificar seu SHA-256**. Nenhum YAML arbitrário é
executado ou interpretado como autorização.

Exige ausência de `source/build`, dos quatro arquivos de release gerados, de
`$WORK` e dos seguintes diretórios em `$RUNNER_TEMP`:

- `download-cache`
- `dependency-work`
- `esp32-coopdiag77`

Cria `$WORK/adapter`, `$WORK/context.json`, `$WORK/state.json` e
`$WORK/evidence`. O adapter é produzido por `benchmark_adapter.materialize`,
com os diffs completos e hashes próprios. Somente as adaptações já revisadas de
contexto/raiz/identidade são permitidas. O wrapper conserva todas as linhas
originais, exceto as duas chamadas de scope redirecionadas ao adapter externo.

Acrescenta ao arquivo real `GITHUB_OUTPUT`:

- `cache_key`
- `artifact_name`
- `evidence_dir`
- `lock_sha256`

O lock está em `dependencies/downloads.lock.json`; o hash confiável e o
contexto vêm de `dependencies/PIN.json`, fora do cache. A chave completa é:

```text
cache_policy.exact_cache_key(lock) + "-run-" + GITHUB_RUN_ID + "-attempt-" + GITHUB_RUN_ATTEMPT
```

Assim, as duas pernas da mesma execução/tentativa compartilham a chave, mas
outras execuções/tentativas não podem aquecer a medição por prefixo. O nome do
artefato é `benchmark-{sourceSHA}-{orchestratorSHA}-{runID}-{attempt}-{phase}`.

### 2. Restaurar e instalar dependências

O workflow executa `dependencies/install_dependencies.py install` em um step
separado, com `--phase`, `--cache-root "$RUNNER_TEMP/download-cache"`,
`--workspace "$RUNNER_TEMP/dependency-work"`, o lock SHA fornecido por prepare,
run ID/attempt reais e `--report "$WORK/evidence/dependencies.json"`. `prepare`
também é um alias aceito pelo helper. O helper pode exportar `GITHUB_ENV` e
`GITHUB_PATH`; `qualify` confirma os caminhos e os configura novamente no
ambiente dos subprocessos, sem modificar `GITHUB_*`.

A perna fria não faz restore e entrega `--restored-key` vazio ao helper. A
perna quente deve restaurar somente a chave exata de prepare/cold e exigir hit
exato **antes da instalação**. Nenhum fallback de prefixo, estado compilado,
checkpoint, captura, firmware ou resultado de teste é cacheável.

O helper grava os caminhos de Python/CLI/config/tools, versões efetivamente
verificadas, contexto, lock, chave, estado de hit/restore, número de arquivos,
bytes e tempos monotônicos de instalação/verificação. Um relatório ausente,
incompleto, divergente ou marcado como falha não é aceito por `qualify`.

### 3. Qualify

O step de qualificação fornece:

```text
cold: BENCHMARK_CACHE_HIT=false, BENCHMARK_CACHE_KEY=""
warm: BENCHMARK_CACHE_HIT=true,  BENCHMARK_CACHE_KEY=<chave realmente restaurada>
```

`qualify` consome o relatório existente e **não instala dependências uma segunda
vez**. Revalida os arquivos do cache pela allowlist completa, tamanho e SHA-256;
confere os caminhos e versões instaladas; exige que os diretórios dos builds e
resultados do produto ainda não existam. O estado passa de `prepared` para
`running`, e depois para `passed` ou `failed`. Não há opção de resume/retry no
mesmo workspace, mesmo depois de uma falha.

Executa, nesta ordem:

1. O wrapper externo derivado de `tools/test_coopdiag77.sh`, com cwd na fonte.
   Permanecem todos os testes de configuração originais, preparação, agregado
   de 157 grupos com jobs 3, fixtures sintéticas originais, capturas e checks de
   paridade. O rótulo do agregado continua `ci-${GITHUB_SHA}` real do
   orquestrador; a identidade da fonte é registrada separadamente.
2. Verificação do summary, source-manifest, plano completo, 157 checkpoints e
   hashes dos logs. Exige exatamente 157 linhas `RUN`, nenhuma `RESUME skip`,
   zero grupos omitidos, LSan obrigatório e o inventário revisado.
3. Scope, versão CLI, lista do core e build default ON. Preserva exatamente
   `arduino-cli compile --verbose --jobs 4`, o FQBN completo revisado,
   `--export-binaries=false` e `firmware/RPG_POKET_2`. ON **não** recebe define
   de coop. Saída externa em `$RUNNER_TEMP/esp32-coopdiag77/flag-1`.
4. Build forced OFF com o único override original
   `compiler.cpp.extra_flags=-DRPG_ENABLE_COOP_TRAINING=0`, em `flag-0`; copia o
   binário de ON para `source/build/RPG_POKET_2.ino.bin`; executa scope.
5. Os cinco comandos originais da etapa de bytes finais:
   `release_coopdiag77_manifest.py`, `verify_coopdiag77_release.py`,
   `tests/coopdiag77_release_verifier.py`, `test_coopdiag77_release.py release`
   e o mesmo teste com `--sanitizers`. Executa scope novamente.
6. `record_identity.py` adaptado, com `--build-root` externo e `--tools-dir`
   verificado pelo helper. Permanecem todos os checks originais de símbolos,
   flags efetivas, linkagem, orçamento, imagem ESP, linhagem, OTA e capturas.
   Somente o envelope de identidade é de benchmark.
7. Confere novamente adapter/contexto e hashes dos três arquivos de release;
   copia as evidências e somente então escreve `measurement.partial.json`.

O runner não trata o wrapper host, nem um build individual, como conclusão da
qualificação. Falhar em qualquer comando impede os gates posteriores e a
emissão do relatório parcial de sucesso.

## Medições e evidência

Cada subprocesso gera um log próprio e uma entrada em `evidence/commands.json`
com argv, cwd, início monotônico, duração em ns e exit code. As fases agregadas
são gravadas em `phase-times.json`. A saída original do wrapper e dos builds
continua também nos caminhos de logs usados pelos verificadores do produto.
`logs/dependency-install.log` preserva o log da instalação separada.

As fases internas de `measurement.partial.json` são:

- `dependency_install`: duração informada pelo helper
- `dependency_verify`: duração do helper mais revalidação do runner
- `aggregate`: wrapper completo, incluindo capturas
- `esp32_on`: scope inicial, consulta do toolchain e build ON
- `esp32_off`: build OFF, cópia do binário ON e scope
- `final_bytes_ota`: cinco comandos de bytes/OTA mais scope
- `identity`: registrador completo adaptado

Todos os tempos usam relógio monotônico. A soma dessas fases não pretende
abranger o job inteiro: checkout, preparação, staging, transporte e overheads
pertencem à duração total real do job. O collector acrescenta restore, save,
verificação imediatamente antes do save e upload usando os steps Actions, e
registra o tempo end-to-end do job. Valores negativos de economia permanecem
visíveis. Nenhuma economia de publicação é inferida.

Schema parcial:

```text
schema: 1
kind: benchmark-partial
phase: cold | warm
benchmark_only: true
cannot_publish: true
context: <contexto completo do adapter>
identity: <campos exatos de compare_benchmark.IDENTITY>
cache: {key, benchmark_salt, hit, restored, contents_verified, forbidden_paths, bytes}
gates: <campos exatos de compare_benchmark.GATES, todos passed após sucesso real>
coverage: {groups:157, skipped:0, resumed:0, lsan:detect_leaks=1,
           asan:true, ubsan_halt_on_error:true, assertions:true}
phases_ns: <sete fases internas acima>
release_files: {firmware.bin:{bytes,sha256}, artes.pak:{bytes,sha256}, manifest.json:{bytes,sha256}}
```

O diretório enviado como artefato contém:

- `measurement.partial.json`, somente após sucesso integral
- `dependencies.json`, `benchmark-context.json`, `runner-status.json`,
  `commands.json`, `phase-times.json` e `logs/`
- `adapter/` com scripts, runtime, manifesto e diffs completos
- `source/release/{firmware.bin,artes.pak,manifest.json,SHA256SUMS}`
- `source/build/` com **todos os padrões de upload originais** que tiveram
  correspondência, inclusive logs/JSONs dos 157 grupos, capturas e paridade
- `source/build/coopdiag77-benchmark-identity.json`; o nome de identidade de
  produção é recusado
- `source-artifact-inventory.json`, com tamanho/SHA-256 dos arquivos copiados

Em falha, o runner preserva os logs e os artefatos parciais já existentes,
registra `runner-status.json` como failed e não cria o measurement de sucesso.
Falhas de staging ficam explícitas em `staging-error.json`; arquivos symlink
não são seguidos. Um artefato parcial é diagnóstico, nunca resultado aprovado.
O collector não deve executar scripts vindos do ZIP de evidência.

## Testes offline

```sh
PYTHONDONTWRITEBYTECODE=1 python3 proposals/ci-acceleration/tests/test_run_benchmark.py
PYTHONDONTWRITEBYTECODE=1 python3 -O proposals/ci-acceleration/tests/test_run_benchmark.py
```

34 testes cobrem identidades/contexto, caminhos e full Git (com leitor Git
simulado), originais/plan/lock alterados, workspaces novos, chaves cold/warm,
relatórios e versões das dependências, symlink legítimo do Python do venv,
receita exata de ON/OFF e OTA, ordem completa com subprocessos simulados,
exit codes/falhas, 157 grupos e proibição de resume, staging de evidências e
vínculo dos bytes finais. Um teste separado executa apenas a rejeição do runner
sob `-O`, antes de qualquer checkout ou gate. Rodar a **suíte de testes** sob
`-O` é suportado; rodar a **qualificação** sob `-O` não é.
