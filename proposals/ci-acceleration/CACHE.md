# Contrato offline de cache de distribuições

## Estado e limite da proposta

O validador `cache_policy.py` é offline: não baixa, instala, extrai, executa,
copia, apaga ou publica arquivos. O lock oficial e instalador separado estão em
`dependencies/`; o workflow proposto continua neste diretório para revisão, sem
alterar Actions ativo. Os testes do validador usam bytes sintéticos não
instaláveis. Firmware e inventário de fontes permanecem intactos.

O objetivo futuro é economizar transferências de dependências, preservando as
instalações limpas e toda a qualificação. Cache não comprova que testes passaram,
não autoriza publicação e não substitui o agregado **157/157**, **LSan=1**, as
compilações ON/OFF, hashes, capturas, verificações OTA ou gates do SHA exato.

## O que pode entrar

Somente arquivos de distribuição individualmente revisados em um lock externo:

- `wheels/<nome>.whl`, obtidos de `https://files.pythonhosted.org/packages/`;
- `archives/<nome>.zip`, `.tar.gz`, `.tar.xz`, `.tar.bz2` ou `.tgz`, provenientes
  de `download.arduino.cc`, `downloads.arduino.cc`,
  `github.com/arduino/arduino-cli/releases/download/` ou `github.com/espressif/`;
- duas exceções exatas já declaradas pelo core ESP32 3.3.12: mkspiffs 0.2.3
  (`igrr/mkspiffs`) e mklittlefs 4.0.2 (`earlephilhower/mklittlefs`). URL, tamanho
  e SHA-256 completos estão fixados em `FROZEN_CORE_DEPENDENCY_URLS`; nenhum
  outro arquivo dessas organizações é admitido por essa exceção.

A lista de domínios é um filtro adicional, **não** aprovação de todo arquivo
hospedado nesses serviços. Cada URL, caminho, tamanho e SHA-256 precisa ser
revisado no lock. Arquivos wheel da distribuição correta, versões, plataforma e
fechamento completo das dependências devem ser conferidos nessa revisão. O
protótipo não infere essas propriedades de nomes de arquivos.

URLs exigem HTTPS, host exato, nome final igual ao arquivo e caminho canônico.
Credenciais, portas explícitas, query strings, fragmentos, escapes percentuais,
backslashes, caracteres de controle e travessia são rejeitados. Um futuro
cliente de download não pode tratar redirecionamentos como permissão implícita
para outros hosts; essa política e os destinos finais precisam de revisão antes
de qualquer ativação. Este protótipo não realiza requisições de rede.

São proibidos executáveis instalados ou soltos, ambientes Python, `site-packages`,
árvores de core/toolchain já extraídas, `~/.arduino15` inteiro, índices mutáveis,
`build`, resultados, `release`, checkpoints, logs, `libcache` e cache global de
compilação. Archives oficiais podem conter executáveis de distribuição; apenas
os bytes do archive permanecem no cache, sem extração. Modos executáveis em
qualquer arquivo do próprio cache também são rejeitados.

## Lock externo e âncora de confiança

`load_trusted_lock` exige um arquivo regular externo à raiz restaurada e seu
SHA-256 esperado, ambos fornecidos independentemente do cache. O SHA esperado
precisa vir de configuração revisada, não de um arquivo restaurado nem de um
hash calculado a partir de um lock recém-encontrado dentro do cache.

O lock é JSON com campos exatos:

- `schema`: inteiro `1`, nunca booleano;
- `context`: todos os nove campos descritos abaixo, sem campos extras;
- `artifacts`: lista não vazia de objetos com exatamente `path`, `url`, `bytes`
  (inteiro positivo) e `sha256` (64 caracteres hexadecimais minúsculos).

Não há comando para gerar ou completar um lock a partir de cache, instalação
existente ou restore. A preparação posterior precisa ser independente: resolver
as distribuições oficiais para as versões/plataformas pretendidas, obter e
conferir metadados e bytes oficiais, revisar URL/tamanho/SHA e o fechamento das
dependências, então aprovar o lock e seu digest como entradas externas.
O fechamento real está documentado em `dependencies/README.md`, com metadados
oficiais congelados e digest independente. Não substitua esses pins pelos
hashes das fixtures sintéticas dos testes.

O leitor rejeita JSON com chaves duplicadas, NaN/Infinity, campos desconhecidos,
listas vazias, caminhos repetidos ou que colidem sem distinguir maiúsculas,
travessia e nomes reservados. Limites defensivos: 2 MiB por lock, 256 entradas,
8 GiB por distribuição e 32 GiB no total. Links simbólicos em qualquer
componente do caminho do lock e hardlinks também são rejeitados.

Uma chamada local não prova por si só revisão humana ou autenticidade da
configuração: a preservação do lock, do digest esperado, do código verificador
e dos valores de contexto é responsabilidade da futura integração confiável.

## Chave exata e contexto

Os nove campos obrigatórios são `runner_os`, `runner_arch`, `python_version`,
`arduino_cli_version`, `esp32_core_version`, `pillow_version`, `pyyaml_version`,
`fqbn` completo e `benchmark_salt`. Versões exigem os três componentes exatos;
pré-releases, como `1.5.2-rc.1`, são preservados. O salt começa por `benchmark-`.
O contexto observado/selecionado pelo chamador deve coincidir integralmente com
o contexto do lock; ele não pode ser inferido do cache restaurado.

A chave base tem esta forma:

```
ci-downloads-v1-<benchmark_salt>-<runner_os>-<runner_arch>-<SHA256(contexto JSON canônico)>-<SHA256(bytes brutos do lock)>
```

O experimento acrescenta `-run-<GITHUB_RUN_ID>-attempt-<GITHUB_RUN_ATTEMPT>`
à chave base, isolando cada par cold/warm sem alterar o lock revisado.

Assim, cada versão, o FQBN e todas as opções, OS, arquitetura, salt e qualquer
mudança nos bytes do lock invalidam a chave. Não há restore-prefixes,
`restore-keys`, fallback por versão parcial nem reutilização cross-OS. A chave
real retornada pela restauração deve ser **igual**, byte a byte, à chave
solicitada. O sinal genérico “algum cache foi restaurado” não é suficiente.
Mesmo uma chave exata não dispensa a verificação de todos os bytes.

No workflow observado, ESP32 `3.3.12`, Pillow `11.3.0` e PyYAML `6.0.3` estão
fixados. Arduino CLI `1.5.2-rc.1` é uma **versão observada**, não um pin do
workflow: `arduino/setup-arduino-cli@v2` não fixa a versão da CLI instalada.
O benchmark resolve explicitamente a versão oficial observada e seu commit
em `dependencies/PIN.json`, verifica Python/CLI/OS/arquitetura e registra a
imagem real de cada job. Cold e warm devem coincidir exatamente; nenhuma
comparação presume que `ubuntu-24.04` seja uma imagem imutável.

## Restore, verificação e miss obrigatório

`evaluate_cache` executa esta ordem:

1. Valida o lock externo, seu hash esperado e o contexto completo. Falta ou
   divergência da âncora de confiança gera `PolicyError`: **parar**.
2. Compara a chave efetivamente restaurada com a chave exata. Chave ausente,
   prefixo, extensão ou diferença produz `action=clean_download`.
3. Abre cada componente da raiz sem seguir symlinks. Confere a árvore inteira,
   incluindo diretórios vazios inesperados. Somente os diretórios `wheels`
   e/ou `archives` necessários ao lock podem existir, com os arquivos exatos.
4. Rejeita symlinks em arquivos e diretórios, hardlinks, FIFO, dispositivos,
   arquivos executáveis, diretórios disfarçados de arquivos e qualquer sobra.
5. Confere o tamanho e SHA-256 de **todos** os arquivos. Descritores ancorados,
   `O_NOFOLLOW`, leituras limitadas, identidades e metadados antes/depois ajudam
   a detectar trocas durante a leitura. A raiz também é reaberta e comparada.
6. Só após sucesso integral retorna `hit=true`, `action=use_verified_downloads`.
   Qualquer cache incompleto, inesperado, ilegível ou corrompido vira miss,
   com zero arquivos autorizados para reaproveitamento parcial.

Contrato de saída da CLI:

- `0`: chave emitida, ou cache integralmente verificado;
- `3`: cache miss com JSON `action=clean_download`; **não usar o restore**;
- `2`: entrada de confiança inválida, com `action=stop`; não tentar reparar o
  lock a partir do conteúdo rejeitado.

Um chamador que recupere um miss deve abandonar toda a árvore restaurada e baixar
limpamente para **outra pasta nova e vazia**, usando apenas as URLs do lock
confiável. Também deve verificar os downloads limpos antes de instalar e antes
de salvar um novo cache. Não basta sobrescrever os arquivos que falharam nem
executar o instalador na pasta restaurada. O protótipo só emite essa decisão:
não implementa download, limpeza, instalação ou save. No experimento, miss no warm encerra antes da instalação e exige um novo par
cold/warm. Não há fallback silencioso que possa ser contado como hit.

Exemplo de interface, sem lock ou SHA fictício de produção:

```
python3 proposals/ci-acceleration/cache_policy.py key \
  --lock "$REVIEWED_EXTERNAL_LOCK" --expected-lock-sha256 "$REVIEWED_LOCK_SHA256" \
  --cache-root "$UNTRUSTED_DOWNLOADS_ROOT" --context-json "$EXACT_CONTEXT_JSON"

python3 proposals/ci-acceleration/cache_policy.py verify \
  --lock "$REVIEWED_EXTERNAL_LOCK" --expected-lock-sha256 "$REVIEWED_LOCK_SHA256" \
  --cache-root "$UNTRUSTED_DOWNLOADS_ROOT" --context-json "$EXACT_CONTEXT_JSON" \
  --restored-key "$ACTUALLY_RESTORED_EXACT_KEY"
```

A futura instalação deve ocorrer em diretório limpo, imediatamente após a
verificação, sem processos concorrentes capazes de alterar os downloads.
O verificador não é um sandbox nem protege contra outro processo privilegiado
que modifica arquivos **depois** da verificação. O cache jamais entra em PATH,
PYTHONPATH, variáveis de carregamento ou diretórios executados pelo runner. A
restauração/descompressão do container do serviço de cache exige proteção
própria contra path traversal, antes mesmo de este validador ser invocado.
O suporte seguro deste protótipo depende de operações POSIX com `dir_fd` e
`O_NOFOLLOW`; plataformas sem elas falham fechadas, sem fallback inseguro.

## Escopo de branches e build do Arduino

Um cache criado em `feat/coopdiag77-release` não fica disponível ao publisher
executado em `main`. O GitHub permite leitura do branch atual e do default,
e em PR também do branch base; não permite que o branch pai leia cache de
branch filho. Portanto, um hit quente no candidato não demonstra um hit no
publisher, mesmo que o texto da chave seja idêntico. Cada escopo deve ter seu
próprio cold/warm medido. Fonte oficial:
[GitHub, restrições de acesso a caches](https://docs.github.com/en/actions/reference/workflows-and-actions/dependency-caching#restrictions-for-accessing-a-cache).

A documentação atual do Arduino CLI diz que, ao especificar `compile
--build-path`, o cache global de compilação é ignorado e a compilação completa
fica no caminho indicado. Os workflows locais usam caminhos separados de ON e
OFF. Logo, esta proposta não promete economizar compilação por guardar o cache
global, não introduz `libcache` e não compartilha objetos entre as flags. Fonte:
[Arduino CLI, guia de atualização](https://docs.arduino.cc/arduino-cli/UPGRADING/).

## Testes reproduzíveis

Execute a partir da raiz do repositório, sem instalar dependências:

```
PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover \
  -s proposals/ci-acceleration/tests -p test_cache_policy.py -v
PYTHONDONTWRITEBYTECODE=1 python3 -O -m unittest discover \
  -s proposals/ci-acceleration/tests -p test_cache_policy.py -v
```

Os testes usam apenas biblioteca padrão e diretórios temporários. Cobrem cada
dimensão da chave, corrupção de tamanho e de digest, conteúdo inesperado,
substituição de raiz, mutações durante leitura, symlinks em ancestrais e
subdiretórios, hardlinks, FIFO, JSON ambíguo, limites, URLs maliciosas, lock
forjado no cache, falhas de confiança e códigos de saída. Testes de CLI executam
normal e `-O`; a política não usa statements `assert`.

A revisão independente integrada do lock, instalador e workflow não encontrou
bloqueador técnico conhecido. Após o gate de envio, falta a medição CI
cold/warm no mesmo SHA e escopo, incluindo instalação e transporte reais.
O cache só contém distribuições; cada job instala tudo de novo em pasta limpa.
Estes arquivos não concedem autorização nem ativam nenhuma dessas etapas. A preparação e o benchmark solicitados seguem o escopo aprovado; ativar um publisher requer aprovação específica separada.
