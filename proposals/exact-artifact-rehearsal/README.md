# Ensaio dos bytes exatos: deploy local para revisão

Nenhuma ativação remota foi feita. O pacote local inclui este diretório e sua
cópia de workflow em `.github/workflows/exact-artifact-rehearsal.yml`. Inclui
apenas ajustes de roteamento do benchmark e sua expectativa de teste. Nenhum
comando, teste, build, cache, parser ou gate da qualificação foi reduzido. Main,
fonte de produto, tags, releases e resultados anteriores não são alterados.

## Contrato específico

O ensaio consome o **formato original da qualificação**, não o formato posterior
do benchmark. Não cria um ledger retrospectivo e não inventa variáveis GITHUB.

- Repositório: `Oflodorporto/RPG-POKET-2`, ID `1406710021`.
- Fonte: `6026fb49db4fe874b9c3f0571cbe4fdb9e66adb3`.
- Árvore: `5ad62cddfe052bea09bf99c29989731722088981`.
- Run `38762209168`, tentativa `1`, job `116516146624`.
- Artifact `11990837049`, `113558660` bytes, ZIP SHA256
  `911b02cd4602251a1f6aa1656017282cf97f9a34af0e9e27629eb3341f52a737`.
- Workflow histórico `.github/workflows/coopdiag77-check.yml`; os pins completos
  e hashes individuais estão em `historical-pin.json`, fora do ZIP.

O orquestrador futuro terá seu próprio SHA, run, tentativa e workflow, usando
a branch já autorizada `bench/coopdiag77-cache-promotion`. A fonte continua sendo 6026. É rejeitado um
orquestrador com SHA/run históricos, contexto divergente, checkout sujo ou script
executado fora do checkout corrente. Nunca são substituídas variáveis GITHUB.

## O que executa

1. Faz somente GET na API GitHub, com o token efêmero de leitura do próprio job.
   Usa run e tentativa explícitas, jobs/artifacts totalmente paginados, detalhe
   do artifact por ID, commit e log do job original. Confere repo/fork, workflow,
   evento/branch, SHA/tree/parent, sucesso, os 12 steps, unicidade, janela do job,
   digest/tamanho/expiração. Aquisição superior a cinco minutos falha; workspace
   é novo e não aceita snapshots no caminho Actions.
2. Reutiliza somente o leitor ZIP já revisado de `../ci-acceleration`, sem alterar
   esse código. Confere os 2.652 nomes do inventário e o ZIP inteiro contra pins,
   verifica 1.178 fontes do checkout, plano completo, 157 checkpoints e hashes de
   todos os logs, execução sem resume e LSan1. Liga o firmware aos builds default
   ON e forced OFF e às evidências de linkage. Confere o log original completo,
   inclusive seus cinco comandos finais e duas execuções OTA sobre bytes reais.
3. Copia apenas `firmware.bin`, `artes.pak` e `manifest.json` para `source/release`
   inexistente, usando criação exclusiva. Nenhum arquivo do ZIP vira código.
   Não recompila ESP32 nem chama o gerador de manifesto. O checksum textual
   histórico é verificado como evidência, mas não é promovido como quarto arquivo.
4. Executa os quatro scripts existentes e intactos da fonte confiável:
   - `tools/verify_coopdiag77_release.py`
   - `tests/coopdiag77_release_verifier.py`
   - `tools/test_coopdiag77_release.py release`
   - `tools/test_coopdiag77_release.py release --sanitizers`
5. Usa o Python absoluto corrente com `-B -s -E`, assertions ativas, compilador
   resolvido explicitamente e LSan1 obrigatório. Rejeita overrides de
   compilador/importação e remove tokens dos subprocessos de produto. Registra
   comando real, comando de referência, cwd, exit code, log/hash e monotonic time.
6. Confere igualdade exata dos três arquivos depois dos gates, inclusive após
   falha, e revalida a fonte. Não aceita alterações, extras ou symlinks. Guarda
   observações de main no início/fim como informação; isso não é lease nem aprovação.

Os testes negativos usam fixtures explicitamente sintéticas, somente em memória.
Não são evidência CI e não alteram o ambiente real. `python -O` conserva as
rejeições do validador; a execução real dos gates de produto recusa `-O`.

## Setup host reduzido e transparente

`setup_host.py` projeta exatamente duas wheels do lock já revisado do benchmark:
Pillow 11.3.0 + PyYAML 6.0.3, **7.453.106 bytes**. Valida SHA256 do lock,
requirements, HTTPS oficial PyPI, tamanho e digest de cada wheel. Exige Python
3.12.3 do lock, cria venv novo e instala offline com `--require-hashes`, sem cache,
sem dependências extras e sem baixar os ~1,85 GB de Arduino/core/toolchains.

Download/verificação e criação/instalação do venv têm tempos separados.
A redução de setup faz parte desta arquitetura de promoção; não é hit de cache.
Não se restauram executáveis, resultados, checkpoints ou provas de qualificação.

## Medição e limites

`report.json`, `commands.json`, logs e hashes antes/depois são o resultado.
`total_runner_ns` inclui validação, obtenção, cópia e gates dentro do runner;
checkout, teste dos guards, setup host e upload são separados. O tempo total do
job só pode ser obtido da API após a conclusão. Não somar fases sobrepostas.

O relatório registra imagem, versão, Python e compilador realmente observados.
A qualificação histórica registra imagem `20261004.327.1` e g++ Ubuntu 13.3.0,
mas não fixa integralmente o setup/Python original. Portanto este único ensaio
não constitui comparação controlada com aquele job. Divergência de imagem ou
compilador mantém qualquer comparação inconclusiva. Não há percentual de ganho
nem estimativa de publisher. Não se transforma o resultado em nova qualificação
completa; os 157 grupos são evidência histórica verificada, não rerun.

`cannot_publish: true`, `publish_authorized: false` e
`authenticated_provenance: false` permanecem explícitos. O último campo indica
que um JSON local não é atestado criptográfico independente; no caminho CI,
os metadados são realmente buscados por GET autenticado naquela execução.
O modo local usa snapshots claramente identificados e nunca diz que são atuais.

Um publisher continua fora deste trabalho: exige autorização específica,
consumo único, controle de frescor/concorrência main/tag/release, publicação e
validação dos três downloads públicos, OTA normal/sanitizado e latest. O
workflow proposto apenas arquiva evidências do ensaio, com credenciais de leitura
para conteúdo/Actions, após autorização separada de ativação.

## Testes locais

Na raiz do checkout da proposta:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 proposals/exact-artifact-rehearsal/tests/test_rehearse.py
PYTHONDONTWRITEBYTECODE=1 python3 -O proposals/exact-artifact-rehearsal/tests/test_rehearse.py
```

Smoke explícito, somente com uma fonte limpa, completa e com histórico Git:

```sh
PYTHONDONTWRITEBYTECODE=1 RPG_DETECT_LEAKS=1 \
python3 proposals/exact-artifact-rehearsal/rehearse.py \
  --source /caminho/absoluto/fonte-6026-limpa \
  --work /caminho/absoluto/resultado-novo \
  --local-snapshot /caminho/snapshots-qualificacao \
  --local-commit-json /caminho/commit-api-historico.json
```

Esse modo não faz download e não mede CI. Não aceita GITHUB_ACTIONS=true, não
regenera bytes e não contorna LSan ou comparabilidade do host. O resultado local
completo e suas limitações ficam no relatório de validação entregue com a proposta.

## Roteamento: nenhum par extra no push de ativação

Base fixa da proposta: commit `4c3dd98a5229b10fb523545a2c18997bc75e5d9a`,
árvore `8517355a1dae54547f4a0b70005a8da81be8cdc1`. O resultado PASS da run
`38839042972` é preservado e não é atribuído ao novo commit.

O benchmark passa a observar somente:

- `.github/workflows/ci-acceleration-benchmark.yml`
- `proposals/ci-acceleration/**`

O ensaio observa somente:

- `.github/workflows/exact-artifact-rehearsal.yml`
- `proposals/exact-artifact-rehearsal/**`

Esses paths sozinhos não bastam: o próprio workflow do benchmark muda no push
que ativa os filtros. Portanto os três jobs `cold`, `warm` e `compare` recebem
a condição adicional explícita:

```text
!contains(github.event.head_commit.message, '[exact-rehearsal-only]')
```

A mensagem exata obrigatória do futuro commit de ativação é:

```text
Rehearse exact qualified coopdiag77 artifact [exact-rehearsal-only]
```

Nesse push, o workflow do benchmark pode aparecer na API, mas os três jobs
ficarão **skipped**: nenhum par será iniciado e nenhum PASS novo será alegado.
A run histórica bem-sucedida continua sendo a prova do par anterior. Não há
mudança no coletor ou parser para aceitar skipped como resultado qualificado.
O ensaio não tem esse guard e executará normalmente quando seus paths mudarem,
inclusive com o marcador presente. Não usar `[skip ci]`, `skip-checks` ou outro
skip global, pois isso impediria também o ensaio.

Futuras mudanças reais no benchmark, sem o marcador, continuam disparando seus
mesmos três jobs e a qualificação inteira. O marcador é um modo explícito desse
push, não uma preferência persistente. Não reutilizá-lo em um commit que
pretenda medir novo par. Mudanças só no ensaio não acionam o benchmark, mesmo
sem marcador. Branches diferentes, forks e eventos fora de push são rejeitados.

A suíte compara o hash estrutural dos corpos completos dos três jobs com a
base aprovada, retirando apenas o campo `if`; steps, needs, actions, comandos,
cache e gates permanecem iguais. Também testa o caso crucial de ativação:
paths do benchmark correspondem, guard impede os três jobs e ensaio continua
habilitado. Sem marcador, o mesmo diff ativaria um par e o gate local acusa isso.

## Revisão antes de qualquer envio

O diff de deploy tem dez caminhos: os seis arquivos deste diretório, sua cópia
ativa de workflow, as duas cópias do workflow do benchmark e a atualização
mínima de seu teste de roteamento. Os demais blobs da árvore-base permanecem
idênticos. `rehearse.py` muda apenas o pin de branch em relação à proposta
anterior, mantendo fonte6026, histórico38762209168 e artifact11990837049.

A cópia ativa é somente preparação local. O envio à branch ainda depende do
gate final e da autorização do parent; não houve upload, push ou execução CI.
Antes de enviar, conferir lease do commit/árvore-base, mensagem exata com o
marcador, diff/allowlist e main `4fc221f07b62d6bd26fcf03fa1bb12c8fd577111` intacta.
O resultado remoto deverá ser observado como ensaio, nunca como publicação.
O artifact histórico expira em 29/10/2026; qualquer troca exige revisão explícita.
