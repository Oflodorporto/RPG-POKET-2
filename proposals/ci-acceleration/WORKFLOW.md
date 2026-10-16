# Workflow executável de benchmark isolado

## Estado e ativação

`ci-acceleration-benchmark.yml` é a proposta de workflow. Um arquivo em
`proposals/` não é executado pelo GitHub Actions. A ativação, quando autorizada
e revisada, consiste em copiar os mesmos bytes para
`.github/workflows/ci-acceleration-benchmark.yml` exclusivamente na branch
`bench/coopdiag77-cache-promotion`. O adaptador exige esse caminho e essa ref.
A branch deve nascer de main fresco, preservando seus documentos; somente
`proposals/` e o workflow novo entram no envio. O checkout de produto continua
fixado em 6026fb4, mesmo se main tiver commits posteriores de documentação.
Não há ativação em main, dispatch, workflow_run, pull request ou publicação.

Os testes deste documento são locais e usam fixtures quando simulam a API.
Eles não comprovam execução remota, os 157 grupos, builds ESP32 ou OTA.
Um arquivo YAML válido também não comprova que o runner oferece a toolchain
histórica. Toda versão real precisa passar os guardas de runtime.

## Execução

1. `cold`: runner GitHub-hosted `ubuntu-24.04` novo. Faz os checkouts completos,
   prepara identidade e adaptador, baixa distribuições oficiais travadas,
   instala fora da fonte e executa toda a qualificação. Revalida os arquivos
   do cache imediatamente antes de salvar. Nunca executa restore.
2. `warm`: outro job/runner efêmero, iniciado somente após sucesso de `cold`.
   Repete os checkouts e a preparação. Exige a mesma chave produzida pelo job
   frio, um hit exato e todos os hashes corretos. Instala as distribuições
   verificadas e executa novamente toda a qualificação. Não salva cache.
3. `compare`: precisa dos dois jobs anteriores, mas usa `always()` para
   preservar diagnóstico quando um deles falha. Busca jobs da tentativa exata
   e artifacts pelos IDs produzidos pelos uploads. O coletor verifica a
   identidade e o digest SHA-256 do ZIP antes de ler o relatório. Uma perna
   ausente, incompleta, falha ou com origem incompatível rejeita a comparação.

O workflow não usa matriz nem `cancel-in-progress: true`. Pushes concorrentes
ficam serializados para reduzir interrupções no meio do par. Isso não fixa a
imagem hospedada: a versão real da imagem continua sendo evidência obrigatória,
e diferenças entre cold/warm invalidam o par controlado.

## Separação de caminhos

- `$GITHUB_WORKSPACE/orchestrator`: SHA real de `github.sha`, workflow e helpers
- `$GITHUB_WORKSPACE/source`: fonte `6026fb49db4fe874b9c3f0571cbe4fdb9e66adb3`
- `$RUNNER_TEMP/bench-work`: contexto, adaptador gerado e evidências
- `$RUNNER_TEMP/dependency-work`: venv, CLI e core instalados para esse job
- `$RUNNER_TEMP/download-cache`: apenas `wheels/` e `archives/` do lock
- `$RUNNER_TEMP/comparison`: downloads e diagnóstico do coletor, só no terceiro job

Ambos os checkouts usam `fetch-depth: 0` e `persist-credentials: false`. Os
checkouts são irmãos e nenhum helper é colocado dentro da fonte. A preparação
precisa validar SHA, árvore, parent, inventário, workflow real e ausência
inicial de build, checkpoints ou release gerada antes de materializar o
adaptador. Nenhuma variável de identidade `GITHUB_*` é substituída. O helper
usa os arquivos de comunicação `GITHUB_ENV`/`GITHUB_PATH` apenas para publicar
os caminhos isolados das dependências.

## Contrato dos comandos

Os dois comandos de `run_benchmark.py` recebem `--source`, `--orchestrator`,
`--work` e `--phase cold|warm`:

- `prepare`: valida o contexto real; confere Python 3.12.3, Linux/X64 e os pins;
  materializa o adaptador antes de criar build; escreve os outputs
  `cache_key`, `lock_sha256`, `artifact_name`, `evidence_dir`. O digest do lock
  deve vir de uma constante revisada no orquestrador, nunca do cache.
- `qualify`: lê a evidência do helper e `BENCHMARK_CACHE_HIT`/
  `BENCHMARK_CACHE_KEY`; exige a origem esperada; executa o agregado completo,
  builds ON/OFF, bytes/OTA e identidade. Escreve
  `bench-work/evidence/measurement.partial.json` e todos os logs necessários.
  O relatório é parcial porque o job ainda não concluiu save/upload e limpeza.

`dependencies/install_dependencies.py install` recebe `--phase`,
`--cache-root`, `--workspace`, `--expected-lock-sha256`, `--run-id`,
`--run-attempt`, `--report`, `--github-env` e `--github-path`. Warm recebe também
`--restored-key`. O workspace de instalação é novo. O cache frio também deve
ser novo. O relatório fica em `bench-work/evidence/dependencies.json`.
Warm inválido deve escrever o diagnóstico e retornar erro antes da instalação;
um download limpo de recuperação não pode ser contado como hit quente.

`dependencies/install_dependencies.py verify` recebe `--cache-root` e
`--expected-lock-sha256`, lê o lock externo e revalida o conjunto exato de
wheels/archives antes do save. Extras, links e corrupção não são aceitos.
Não se guardam ferramentas extraídas, objetos, executáveis de teste, builds,
resultados, logs, capturas ou checkpoints no cache.

A chave é a saída de `cache_policy.exact_cache_key` para o lock/contexto
revisado, acrescida de `-run-<run_id>-attempt-<run_attempt>`. O contexto do lock
inclui seu `benchmark_salt`. O sufixo liga o experimento à run/tentativa sem
reescrever o lock ou confiar em um hash fornecido pelo conteúdo restaurado.
Não há restore-keys, fallback por prefixo ou save automático em post-action.

A qualificação mantém CLI 1.5.2-rc.1, core 3.3.12, Pillow 11.3.0,
PyYAML 6.0.3, Python 3.12.3, g++ 13.3 histórico, FQBN revisado,
host jobs=3 e ESP32 jobs=4. Não há instalador móvel de Arduino ou setup-python
que troque a baseline silenciosamente. Lock incompleto, distribuição oficial
indisponível ou versão diferente são bloqueios, não licença para substituir.

## Coleta pós-job e transporte

Os nomes dos jobs são literalmente `cold`, `warm` e `compare`. O coletor
identifica os seguintes nomes de step sem inferir duração pelos logs:

- `Cache restore`: apenas warm
- `Cache save`: apenas cold
- `Upload benchmark evidence`: ambos
- `Install verified dependencies`: ambos

O endpoint consultado é
`GET /repos/{owner}/{repo}/actions/runs/{run_id}/attempts/{run_attempt}/jobs`,
com `per_page=100`, `gh api --paginate --slurp`. A saída preserva todas as
páginas; o coletor deve rejeitar jobs incompletos, duplicados ou de outra
run/tentativa/SHA, normalizar IDs string/int e usar os horários oficiais dos
jobs já concluídos. O compare ainda estar em andamento não invalida o status
concluído dos dois jobs medidos.

Os IDs de artifact vêm de `steps.evidence.outputs.artifact-id`, transmitidos
pelos outputs de cada job. A busca é individual por ID, primeiro metadata,
depois ZIP. Não há descoberta por nome, “última run” ou extração de scripts
baixados. O coletor recebe:

    collect_benchmark.py \
      --cold-id "$COLD_ARTIFACT_ID" --warm-id "$WARM_ARTIFACT_ID" \
      --cold-zip cold.zip --cold-metadata cold.metadata.json \
      --warm-zip warm.zip --warm-metadata warm.metadata.json \
      --jobs jobs.json --output result.json

O coletor deve verificar SHA-256 do ZIP contra `metadata.digest`, ID contra os
outputs esperados, nome, run, branch, SHA do orquestrador e timestamps do
artifact dentro da janela do job correspondente. Ele lê somente o relatório esperado do ZIP,
com limites e validações, sem executar conteúdo do artifact. Esses controles
não são substituídos por um aviso não fatal de digest de uma download action.

O upload é imutável (`overwrite: false`) e usa nomes por SHA, run, tentativa e
fase. O nome normal vem de `prepare`; se a preparação falhar antes de produzir
outputs, existe um nome de diagnóstico próprio da fase. O ZIP inclui o
conteúdo de `bench-work/evidence`, com `measurement.partial.json` na raiz.
Falta de evidência é erro. Uploads são tentados mesmo após falha; se o runner
for encerrado antes de enviar, a ausência fica explícita. Não se afirma que
uma tentativa interrompida preservou todos os logs.

Os tempos de instalação e verificação são medidos no helper/wrapper; restore,
save e upload vêm da API. O tempo total do job inclui os checkouts, setup,
post-actions e demais overheads. Um único par oferece uma observação com
ruído, inclusive economia negativa. Não mede ganho de publicação.

O terceiro job envia apenas JSON/TXT de diagnóstico da comparação, sem repetir
os ZIPs de evidência. Logs adicionais continuam nos artifacts cold/warm.

## Permissões e limites

As únicas permissões do token são `contents: read` e `actions: read`.
O `GH_TOKEN` do fetch vem de `github.token`, somente naquele step; nenhum PAT,
secret adicional, login persistente ou credencial git é criado. Cache e upload
usam a infraestrutura efêmera própria do Actions, sem write no repositório.
Essa fronteira permite salvar cache/evidências do experimento, mas não alterar
fonte remota, tags, releases, main ou permissões.

Nenhum artifact do experimento é candidato promovível. Sucesso da comparação
não autoriza publicação, nem reutilização de checkpoints para pular testes.
Os scripts de produto, testes e workflows de qualificação/publicação existentes
não são editados por esta proposta.

## Pins e fontes oficiais consultadas

Verificados em 2026-10-15, sem depender de resolução futura de tags:

- Checkout, commit `11d5960a326750d5838078e36cf38b85af677262`, observado também
  na baseline 38762209168:
  https://github.com/actions/checkout/commit/11d5960a326750d5838078e36cf38b85af677262
- Upload artifact, commit `ea165f8d65b6e75b540449e92b4886f43607fa02`, observado
  também na baseline:
  https://github.com/actions/upload-artifact/commit/ea165f8d65b6e75b540449e92b4886f43607fa02
- Cache v4.2.3, commit `5a3ec84eff668545956fd18022155c47e93e2684`, tanto
  `restore` quanto `save`:
  https://github.com/actions/cache/releases/tag/v4.2.3
  https://github.com/actions/cache/commit/5a3ec84eff668545956fd18022155c47e93e2684
- Inputs/output oficiais da restauração exata:
  https://github.com/actions/cache/blob/5a3ec84eff668545956fd18022155c47e93e2684/restore/action.yml
- API oficial de jobs por tentativa e paginação:
  https://docs.github.com/en/rest/actions/workflow-jobs#list-jobs-for-a-workflow-run-attempt

`download-artifact` v4.3.0 foi conferido em
`d3f86a106a0bac45b974a628896c90dbdf5c8093`, mas não é usado: a implementação
mantém o ZIP cru para o coletor rejeitar explicitamente divergência do digest.
Fonte: https://github.com/actions/download-artifact/releases/tag/v4.3.0

## Validação local do workflow

    PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover \
      -s proposals/ci-acceleration/tests -p test_benchmark_workflow.py -v
    PYTHONDONTWRITEBYTECODE=1 python3 -O -m unittest discover \
      -s proposals/ci-acceleration/tests -p test_benchmark_workflow.py -v

A suíte faz parse YAML estrito (rejeita chaves duplicadas), verifica permissões,
triggers, ações por SHA, sequenciamento, raízes, cache frio/quente, transporte
por ID e execução somente de código do orquestrador. Confere todos os blocos
bash com `bash -n` e exercita o shell real de hit/miss e download com `gh`
sintético. Requer PyYAML disponível no ambiente local. Não faz chamadas à API,
não baixa dependências nem executa a qualificação durante esses testes.
