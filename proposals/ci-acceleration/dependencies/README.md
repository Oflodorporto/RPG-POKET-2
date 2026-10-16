# Lock oficial de distribuições para o experimento

Preparação de dependências do benchmark isolado. Não muda os workflows de
produto, não publica firmware e não demonstra que a qualificação passou.
Nenhum dos binários deste lock foi baixado ou executado na preparação local.
O primeiro cold em CI ainda precisa verificar todos os bytes reais e executar
as instalações; somente esse CI poderá comprovar a integração completa.

## Escopo fixado e origem

- Ubuntu 24.04, `RUNNER_OS=Linux`, `RUNNER_ARCH=X64`, CPython **3.12.3** do runner.
  Esse patch é uma escolha explícita da nova comparação controlada. Se a imagem
  o mudar, o helper para. Não infere o patch histórico nem usa `setup-python`.
- Pillow 11.3.0 e PyYAML 6.0.3: duas wheels CPython 3.12, glibc/manylinux x86-64;
  nenhum extra ou dependência Python obrigatória adicional.
- Arduino CLI **1.5.2-rc.1**, commit
  `fef6e48df6311752b7bfc0498e53e35f37869560`. O tag oficial aponta a esse commit;
  tamanho/digest do asset Linux64 e checksum publicado concordam.
- Core `esp32:esp32@3.3.12`, todas as **18** dependências de ferramenta indicadas
  no índice oficial da release, incluindo toolchains de outras arquiteturas que
  o comando normal de instalação também instala. Não reduz o core ao ESP32-S3.
- Os cinco builtin tools da CLI também ficam explícitos: ctags 5.8-arduino11,
  dfu-discovery 0.1.2, mdns-discovery 1.1.0, serial-discovery 1.5.2 e
  serial-monitor 0.15.0. O fechamento completo está em `closure.json`.

São **27 arquivos**, **1.847.821.339 bytes** comprimidos. O lock tem SHA-256
`d03e64207ca2c45c81f01ecf64ff8faf230f6552e12c7d96d552d68491c2fa78`.
Não inclui Python, pip ou pacotes do sistema pré-instalados na imagem. A
identidade real da imagem/compiladores continua sendo registrada no benchmark.

Fontes oficiais consultadas em 15/10/2026:

- [Pillow no PyPI](https://pypi.org/pypi/Pillow/11.3.0/json)
- [PyYAML no PyPI](https://pypi.org/pypi/PyYAML/6.0.3/json)
- [Release oficial da CLI](https://github.com/arduino/arduino-cli/releases/tag/v1.5.2-rc.1)
- [Checksums oficiais da CLI](https://github.com/arduino/arduino-cli/releases/download/v1.5.2-rc.1/1.5.2-rc.1-checksums.txt)
- [Tag e commit da CLI](https://api.github.com/repos/arduino/arduino-cli/git/ref/tags/v1.5.2-rc.1)
- [Release oficial ESP32](https://github.com/espressif/arduino-esp32/releases/tag/3.3.12)
- [Índice congelado da release ESP32](https://github.com/espressif/arduino-esp32/releases/download/3.3.12/package_esp32_index.json)
- [Índice oficial Arduino](https://downloads.arduino.cc/packages/package_index.json)

Os snapshots estão em `sources/`. `sources.lock.json` registra URL, tamanho,
SHA-256 e representação. Metadados da API GitHub foram normalizados como JSON;
índices, JSON do PyPI e checksums preservam os bytes recebidos. O SHA-256 do
índice ESP32 e do arquivo de checksums da CLI foi conferido contra os digests
dos assets publicados na API GitHub. O índice geral Arduino é um snapshot
datado de uma URL mutável, **nunca atualizado durante o experimento**.

Os arquivos mkspiffs 0.2.3 e mklittlefs 4.0.2 são hospedados respectivamente
em `igrr/mkspiffs` e `earlephilhower/mklittlefs`, fornecedores selecionados pelo
próprio índice oficial Espressif. A política permite somente os dois URLs,
tamanhos e hashes exatos revisados, não esses namespaces inteiros.

## Índices locais explícitos, fora do cache

`generate_lock.py` não acessa a rede. Confere os snapshots, resolve o fechamento
e reproduz o lock, os pins, requirements e dois índices **derivados** mínimos:

- `frozen/package_index.json`: somente o core, as 18 ferramentas e os cinco
  builtins, com versões/sistemas explícitos e URLs/hashes oficiais intactos;
- `frozen/library_index.json`: índice local explicitamente vazio, pois o core
  fixado não declara bibliotecas externas. Não é apresentado como índice
  oficial completo e não serve para instalar outras bibliotecas.

Esses JSONs são entradas revisadas do checkout, nunca itens do cache. São
colocados no data-dir novo antes da primeira inicialização da CLI. Não se chama
`core update-index`. Isso impede a seleção implícita do builtin mais recente e
a atualização automática inicial de índices. A CLI funciona normalmente com
índices locais sem assinatura; nossa âncora é o checkout/lock revisado e seus
digests, não uma assinatura Arduino sobre o índice derivado.

O comportamento foi conferido no [código da CLI fixada](https://github.com/arduino/arduino-cli/blob/fef6e48df6311752b7bfc0498e53e35f37869560/commands/instances.go):
`firstUpdate` só baixa índices ausentes; a inicialização instala a última versão
visível de cada builtin. O [código de download](https://github.com/arduino/arduino-cli/blob/fef6e48df6311752b7bfc0498e53e35f37869560/internal/arduino/resources/download.go)
confere tamanho/checksum do archive já presente antes de baixá-lo novamente.
Notificações de atualização ficam desativadas. O instalador passa ainda um
proxy local fechado à CLI como defesa contra requisições imprevistas; isso não
é um sandbox de rede nem alegação de isolamento do sistema operacional.

Esta fixação de setup deve valer igualmente para cold e warm. Ela requer uma
**nova baseline controlada**: não transforma os tempos históricos do setup
mutável em uma perna cold equivalente.

## Interface e fluxo

Verificação/regeneração offline:

```
python3 proposals/ci-acceleration/dependencies/generate_lock.py
# --write somente durante preparação/revisão explícita, jamais em cache miss.
```

`PIN.json` e `SHA256SUMS` registram os digests revisados fora do cache. O caller
passa `--expected-lock-sha256` como entrada confiável. Nunca derive esse valor
do cache restaurado. O helper confere também a reprodução dos índices/lock.

```
python3 proposals/ci-acceleration/dependencies/install_dependencies.py key \
  --expected-lock-sha256 "$LOCK_SHA" --run-id "$GITHUB_RUN_ID" \
  --run-attempt "$GITHUB_RUN_ATTEMPT"

python3 proposals/ci-acceleration/dependencies/install_dependencies.py install \
  --phase cold --cache-root "$RUNNER_TEMP/download-cache" \
  --workspace "$RUNNER_TEMP/dependency-work" \
  --expected-lock-sha256 "$LOCK_SHA" --cache-key "$EXACT_KEY" \
  --report "$RUNNER_TEMP/evidence/dependencies.json" \
  --github-env "$GITHUB_ENV" --github-path "$GITHUB_PATH"

python3 proposals/ci-acceleration/dependencies/install_dependencies.py verify \
  --cache-root "$RUNNER_TEMP/download-cache" --expected-lock-sha256 "$LOCK_SHA"
```

`prepare` é alias de `install`. `--run-id/--run-attempt` podem ser fornecidos;
caso contrário são lidos do ambiente Actions. A chave de transporte é a chave
exata de `cache_policy` acrescida de `-run-ID-attempt-N`, sem restore-prefixes.
Cold e warm do mesmo experimento usam a mesma chave; outro run/attempt não a usa.

Cold exige cache-root inexistente. Baixa em pasta nova, verifica tudo, move
atomicamente para cache-root e instala de cópias separadas revalidadas.
Warm exige a chave de transporte inteira e a árvore integral válida. Em miss,
corrupção ou arquivo extra, grava `action=clean_download`, sai com código 3 e
**não instala nem salva nada**. Para a comparação controlada, isso exige iniciar
outro experimento cold/warm; não rotula um download de recuperação como warm.

O download aceita HTTPS nos URLs do lock e, para releases GitHub, somente o
redirecionamento conhecido a `release-assets.githubusercontent.com`. Cada hop é
validado, sem downgrade, credenciais ou host arbitrário. Bytes são lidos com
limite, tamanho e SHA-256 e só então promovidos de `.partial`. Erros HTTP
transitórios podem tentar novamente a mesma URL até três vezes.

Instalação cria venv sem bootstrap pip, usa o pip já presente no runner com
`--python` e `--no-index --no-deps --require-hashes --only-binary=:all:`.
Extrai somente o binário regular `arduino-cli` do tar oficial verificado.
Archives de core/tools são copiados para um staging novo, com nova verificação;
a CLI executa `core install esp32:esp32@3.3.12` normalmente. O report registra
identidades realmente observadas e as durações, incluindo cópia/verificação.

O report de sucesso tem `schema=1`, `status=verified_and_installed`, contexto,
`cache_key`, `cache_hit`, `lock_sha256`, `cache_bytes`, `benchmark_salt`,
`dependency_install_ns`, `dependency_verify_ns`, `python`, `cli`, `config`,
`tools_dir`, `venv`, e os detalhes `cache`, `paths`, `tool_identity`.
`python` aponta ao symlink legítimo `venv/bin/python3`, sem resolver ao Python
do sistema. Os arquivos de ambiente exportam venv/bin, bin da CLI e config.
O caller deve usar a config explícita em todos os comandos da CLI.

## Limites e revisão

Instalações, data-dir, staging, venv, resultados e logs nunca entram no cache.
O cache nunca entra em PATH/PYTHONPATH. Há somente distribuições comprimidas
oficiais e wheels; fonte, testes, artefatos e os 157 grupos seguem inalterados.
O runner precisa de espaço para cache, staging, ferramentas extraídas e dois
builds. Não se promete economia no primeiro cold ou sobre o custo de extração.

A raiz do cache e as áreas de instalação devem permanecer sem escritores
concorrentes, conforme `CACHE.md`. O helper não é sandbox e a confirmação de
digest não protege contra um processo privilegiado alterando bytes depois da
verificação. O downloader não obtém credenciais; redirects não concedem acesso
a outros hosts. Os testes usam somente metadados e fixtures sintéticas, sem
downloads grandes, execução de CLI/toolchains, CI ou publicação.
