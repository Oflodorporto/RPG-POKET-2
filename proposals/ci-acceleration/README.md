# Aceleração CI: experimento preparado para revisão

Fonte de produto: `6026fb49db4fe874b9c3f0571cbe4fdb9e66adb3`, árvore `5ad62cddfe052bea09bf99c29989731722088981`.

Este patch está inteiramente em `proposals/`. Não altera nenhum workflow ativo, fonte do firmware, teste do jogo, inventário congelado, credencial, permissão, tag ou release. A publicação concluída permanece independente deste trabalho. Ainda não há workflow remoto de benchmark ou publisher novo.

## O que foi implementado

1. Contrato de promoção offline anterior adaptado para as 156 tarefas atuais mais preparação: `../qualified-artifact-promotion/`. Mantém SHA/árvore, inventário, plano/commands, hashes dos logs, gates/sanitizers, ID/run/tentativa, digest/tamanho, frescor de main, tag/release, validade e rejeições de ZIP/JSON. Sucesso nunca autoriza publicar.
2. Cache de distribuições verificadas: `cache_policy.py`. O lock deve vir da fonte confiável revisada e seu hash deve ser fixado fora do cache. Somente wheels/archives oficiais listados podem ser reutilizados. Todo conteúdo é verificado antes de ser usado; faltas, extras, corrupção e links não viram dependências aceitas. Não existe lock de produção inventado ou capturado de um cache desconhecido.
3. Adaptador explícito de benchmark: `benchmark_adapter.py`. Distingue checkout de fonte e checkout do orquestrador. Mantém os GITHUB_* reais, conserva todas as validações de produto e gera patches completos auditáveis para somente os pontos de identidade. Mais detalhes em `BENCHMARK_IDENTITY.md`.
4. Comparador frio/quente: `compare_benchmark.py`. Recusa pares com mudança de fonte, árvore, inventário, plano, adapter, lock, versões, FQBN, paralelismo ou imagem de runner; também recusa resume, skip, sanitizer enfraquecido, gate ausente, falso hit e origem de cache não vinculada ao job frio. Inclui overhead de restore, verificação, save e upload; preserva resultados negativos.
5. Lock oficial fechado em `dependencies/`: 27 distribuições, 1.847.821.339 bytes, SHA-256 `d03e64207ca2c45c81f01ecf64ff8faf230f6552e12c7d96d552d68491c2fa78`. A CLI oficial 1.5.2-rc.1 foi encontrada no commit fef6e48df6311752b7bfc0498e53e35f37869560. Índices derivados congelados e instalador novo impedem dependências implícitas/mutáveis. O cache nunca guarda instalações extraídas.
6. `run_benchmark.py` executa o contrato completo em checkouts reais separados e grava tempos, comandos, evidências e identidade própria. `ci-acceleration-benchmark.yml` propõe os jobs cold → warm → compare, sem publicação e com Actions fixadas por SHA. `collect_benchmark.py` confere IDs, metadados API, digest externo, ZIP, 157 checkpoints e bytes finais antes de comparar.
7. Plano atual congelado em `reviewed-plan.json`, contrato em `source-contract.json`, baseline medido em `baseline-timings.json` e auditoria de evidências reais em `existing-evidence-audit.json`. A observação do publisher concluído fica separada em `publisher-baseline-timings.json`.

## O que já foi conferido com dados reais

A qualificação 38762209168, tentativa 1, corresponde à fonte acima. Foram conferidos 1.178 hashes de fontes, todos os 157 checkpoints e seus comandos/logs contra o plano atual. O verificador semântico original passou sobre o firmware, arte e manifesto baixados daquela qualificação. Isso é auditoria de evidência existente, não rerun do jogo ou nova qualificação.

Os logs individuais dos checkpoints somam 246.539 bytes; o maior tem 35.602 bytes. Payloads somam 10.436.812 bytes, então o limite experimental de cápsula de 64 MiB parece plausível. Esse limite vale só para a cápsula estreita experimental: o artefato completo do publisher mediu 113.558.644 bytes e 2.652 arquivos. O coletor do benchmark permite ZIP até 8 GiB e 16 GiB expandidos, com limites por entrada e leitura em fluxo. Não truncar evidências para fazê-las caber.

Tempos REST do job (fim menos início, sem fila): agregado 42m29s; instalar core 57s; builds ON/OFF 2m43s; bytes/OTA 5m06s; job 51m43s. A leitura anterior por limites internos dos logs deu core 56s e job aproximado 51m39s. Os dois métodos estão separados no JSON. Esta única observação histórica não é comparação controlada e não demonstra ganho de cache.

O publisher 38770945726 terminou com sucesso: 55m24s de job; agregado 41m31s, core 56s, builds 2m38s, bytes/OTA pré-publicação 5m01s e verificação dos downloads públicos 4m45s. O rebuild mudou 67 bytes do firmware, restritos a timestamp de compilação, digest ELF e checksums; instruções/dados fora desses campos permaneceram idênticos na auditoria independente. Logo, a baseline antiga não publicou firmware byte-idêntico ao candidato. Uma promoção futura deve conservar exatamente o arquivo qualificado, sem reinterpretar essa diferença histórica como igualdade de hashes.

## Experimento proposto para revisão

- Um workflow somente leitura, em `bench/coopdiag77-cache-promotion`, com cold e warm em jobs efêmeros distintos e sequenciais. Sem trigger de main, sem workflow_run, sem contents:write, sem clobber ou publicação.
- Dois checkouts irmãos: orquestrador em seu SHA real e fonte em 6026fb4, ambos completos, sem credenciais persistidas. Adaptador e scripts de controle ficam fora da fonte. A qualificação original não aceita arquivos extras no inventário e não deve ser editada para fazer o experimento passar.
- CLI oficial 1.5.2-rc.1, core 3.3.12, Pillow 11.3.0, PyYAML 6.0.3, Python 3.12.3, FQBN e comandos estão fixados. Os bytes das distribuições serão conferidos contra hashes oficiais antes de qualquer instalação. Índices mínimos congelados e Python exato são escolhas explícitas para uma nova baseline controlada; não fazem a observação histórica virar perna cold equivalente.
- Cold ignora qualquer cache anterior; gera cache novo em chave vinculada ao run/attempt exclusivo do experimento. Warm exige hit exato dessa chave e a origem do job frio. Não usar restore-keys. Não restaurar build/, objetos, resultados, executáveis host, logs ou checkpoints.
- Ambos executam o wrapper completo, 157 grupos sem resume, LSan=1, ASan/UBSan/assertions, capturas, budgets/linkage ESP32, builds ON padrão e OFF explícito, verificação semântica e todos os testes OTA em bytes reais. Conservar jobs host=3 e jobs ESP32=4.
- Registrar tempos monotônicos por fase, falhas/exitcodes, versão/imagem real do runner, tamanho do cache, origem/hit, hashes finais, bytes transportados, download/upload e tempo do job incluindo overhead. Se imagem ou toolchain diferirem entre cold/warm, não chamar isso de ganho controlado.
- Artefatos do benchmark usam nome próprio com SHA fonte, SHA orquestrador, run, tentativa e fase/job. Cold/warm da mesma run não podem disputar o mesmo nome imutável. Eles nunca recebem o nome release-candidate nem a procedência de uma qualificação autorizada; o verificador de promoção existente não deve aceitá-los como tal.
- Em ensaio separado de promoção, baixar a cápsula por ID imutável de run/tentativa e verificar digest do transporte, cápsula e payloads. Reexecutar semântica e OTA normal/sanitizado sobre os mesmos bytes; não reconstruir firmware ou manifesto. Este ensaio continua incapaz de publicar e não simula sucesso de downloads públicos de uma release inexistente.
- Comparar ensaio com repetição completa somente quando fonte, escopo, versões e método coincidirem. Cache pode até piorar o tempo líquido, e isso deve aparecer. Uma dupla inicial mede uma observação; mais pares serão úteis se o efeito for pequeno ou houver ruído relevante.

## Fronteira de confiança da promoção

Artefato e cache são mecanismos diferentes. A cápsula pode transportar os checkpoints/logs como evidência da qualificação específica, mas eles nunca são reutilizados para pular testes numa nova qualificação. A promoção futura só evita a repetição do agregado/build no publisher porque referencia uma qualificação exata já concluída e autorizada.

A autorização deverá fixar SHA/árvore, versão/build/tag nova, run/tentativa, artifact ID/digest e hashes finais. Deve ser autenticada fora do ZIP, de uso único, e consumida com protocolo de concorrência revisado para main/tag/release. Checar main logo antes ajuda, mas não torna atômicas duas chamadas API. Branch/run verde ou JSON approved=true não substituem autorização.

O job que inspeciona/executa gates continua read-only. Um publisher futuro recebe somente os três arquivos aprovados e um registro autenticado, não executa scripts do artefato e não restaura cache produzido por outra origem. Após criar a release, deve baixar os três arquivos publicados, conferir bytes/digests e tag→commit, repetir OTA normal/sanitizado sobre os downloads e validar latest. Falha parcial deve ficar explícita e exigir reconciliação, sem mover tag/latest ou sobrescrever assets automaticamente.

## Gate antes de enviar/executar a branch de benchmark

O código, lock oficial, workflow e coletor estão preparados localmente. A revisão independente integrada concluiu sem bloqueador técnico conhecido (174 + 30 testes, normal e -O). Falta o gate de apresentação/revisão deste diff antes do envio. Após esse gate, criar a branch a partir de main fresco, preservando seus documentos; adicionar somente `proposals/` e copiar o workflow proposto para `.github/workflows/ci-acceleration-benchmark.yml` na branch `bench/coopdiag77-cache-promotion`, mantendo os workflows originais byte-idênticos. Nenhum commit artificial da fonte local deve ser criado: o Actions buscará o commit remoto real fixado.

O primeiro CI validará de fato downloads/instalação, duas qualificações completas e a coleta. Testes locais usam metadados reais e fixtures offline, não demonstram que a instalação ou o produto passaram. O par deve custar aproximadamente duas qualificações completas; se falhar, não haverá relatório de sucesso nem conclusão de ganho.

Primeiro medir somente cache. Depois preparar/executar ensaio read-only de promoção sobre os bytes qualificados, com medição separada. A preparação e testes desse ensaio já estão no escopo autorizado; ativar publisher continua sendo outra decisão.

## Bloqueios adicionais antes de ativar um publisher

- Adaptador API autenticado/paginado, run/tentativa/workflow/repo/branch exatos e validação de artifact ID/digest/expiração.
- Autorização autenticada de uso único e concorrência main/tag/release demonstrada; fresh main isolado não resolve toda corrida.
- Selador real e evidências completas dos gates finais; artefato atual não é presumido compatível.
- Testes de falha, retry, replay, TOCTOU/links, bytes antes/depois, downloads/latest e revisão independente.
- Apresentação e aprovação específica do novo publisher para próxima versão. Este experimento não ativa main nem reutiliza a autorização da publicação corrente.

## Testes locais

    PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover -s proposals/ci-acceleration/tests -v
    PYTHONDONTWRITEBYTECODE=1 python3 -O -m unittest discover -s proposals/ci-acceleration/tests -v
    PYTHONDONTWRITEBYTECODE=1 python3 proposals/qualified-artifact-promotion/tests/test_verify_candidate.py
    PYTHONDONTWRITEBYTECODE=1 python3 -O proposals/qualified-artifact-promotion/tests/test_verify_candidate.py

Fixtures são claramente sintéticos. A suíte do adaptador pode rodar em -O, mas o runtime dos gates originais rejeita -O para preservar suas assertions.
