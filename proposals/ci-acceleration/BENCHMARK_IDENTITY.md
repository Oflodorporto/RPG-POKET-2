# Proposta isolada: identidade de fonte e de orquestrador

Status: protótipo local, somente preparação e testes com fixtures. Nenhum
workflow foi criado, enviado ou executado. Não qualifica uma publicação.

## Por que o adapter é necessário

O contrato original combina duas identidades que coincidem no fluxo publicado:
`HEAD` da fonte e `GITHUB_SHA` do workflow. O scope ainda exige que a fonte seja
filha única de `c8738eb67bde3817feaab9f3a53c30607396a5f5`; o registrador aceita
somente push em main ou feat/coopdiag77-release. A configuração testa a sequência
exata dos workflows atuais. Um workflow isolado não pode fingir ser esse fluxo.

Este protótipo deixa todos os arquivos originais intactos e gera cópias externas
com mudanças explícitas exclusivamente de contexto, raiz e envelope de saída.
Nenhuma variável `GITHUB_*` é sobrescrita. O publisher atual continua intacto e
não é consumidor dos resultados deste benchmark.

## Contrato fixo

- Repositório: Oflodorporto/RPG-POKET-2
- Fonte: `6026fb49db4fe874b9c3f0571cbe4fdb9e66adb3`
- Árvore: `5ad62cddfe052bea09bf99c29989731722088981`
- Pai único: `c8738eb67bde3817feaab9f3a53c30607396a5f5`
- Orquestrador futuro: push na branch exata `bench/coopdiag77-cache-promotion`, workflow próprio em
  `.github/workflows/ci-acceleration-benchmark.yml`; SHA distinto da fonte
- SHA, árvore, ref, workflow/ref, workflow SHA/digest, repository ID, run e
  tentativa são registrados separadamente e conferidos contra checkout/env
- O rótulo de agregado permanece `ci-${GITHUB_SHA}`, nomeando o orquestrador
  verdadeiro. A identidade da fonte vem de HEAD, árvore e manifesto de arquivos
- Checkouts source, orchestrator e diretório dos adapters são separados, sem
  sobreposição e sem redirecionamento de raiz por symlink

A referência exata `refs/heads/bench/coopdiag77-cache-promotion` não é autoridade de release. O arquivo de contexto
é uma entrada local de expectativa, e não aprovação nem atestado independente.
O workflow futuro precisa fixar seus próprios pins confiáveis fora do artefato.

## As únicas mudanças geradas

1. `test_coopdiag77.sh`: troca exatamente as duas chamadas do scope pelo caminho
   absoluto do adapter. Nenhuma tarefa, argumento, ordem, captura ou gate muda.
2. `check_scope.py`: inclui verificação explícita do contexto; resolve ROOT para
   o checkout source intacto; passa o SHA da fonte validada ao argumento `ci_sha`
   da mesma função `validate_ancestry`, conservando o modo CI real. Todas as
   assertions originais, inclusive ancestralidade, inventário e conteúdo, ficam.
3. `record_identity.py`: substitui as três assertions de contexto original por
   verificação equivalente ao contrato independente, HEAD da fonte e LSan.
   Todas as validações do agregado, 157 checkpoints, comandos, hashes de logs,
   capturas, builds ON/OFF, flags, orçamento, linhagem e bytes finais permanecem.
   Todas as funções auxiliares são cópias AST-idênticas. A saída passa a
   `build/coopdiag77-benchmark-identity.json` com `benchmark_only=true`,
   `cannot_publish=true`, `published=false`, contexto e hash do manifesto adapter.
   O campo antigo branch vira orchestrator_ref, evitando atribuí-lo à fonte.

Os três originais têm SHA-256 fixos no materializador. Substituições exigem
contagem exata de âncoras. `adapter-manifest.json` contém os diffs unificados
completos e hashes originais/gerados. Ao executar um adapter Python, o runtime
reconstrói as mesmas transformações a partir dos originais pinados e compara
cada byte; editar um arquivo e atualizar seu hash no manifesto não basta.
O runtime também compara sua própria cópia à versão no checkout limpo do
orquestrador. Código e manifesto extras, symlinks e alterações são rejeitados.

O runtime exige Python sem otimização, pois os gates originais usam assertions.
O materializador e suas verificações usam erros explícitos, funcionando também
sob `python3 -O`. A execução dos gates adaptados sob `-O` é recusada.

## Integração futura necessária, ainda não implementada

1. Obter os dois checkouts genuínos e completos. Materializar adapters fora de
   ambos com contexto verdadeiro. Não usar uma árvore staged como commit 6026.
2. A materialização inicial rejeita qualquer source/build existente e os quatro
   arquivos de release gerados, inclusive symlinks pendentes. Isso impede
   reutilizar checkpoints, builds ou bytes de outra perna. As verificações
   posteriores permitem os resultados novos desta execução.
   Executar o wrapper externo com cwd no checkout source. Os testes originais
   continuam lendo e verificando os workflows e scripts originais intactos.
3. Copiar a etapa de compilação existente, trocando apenas a comparação de HEAD
   para o SHA source validado e as chamadas scope para o adapter. Preservar CLI,
   core 3.3.12, compilador esp-x32/2601, FQBN integral, opções, jobs 4, ON sem override,
   fallback OFF e diretórios de build externos separados.
4. Rodar toda a etapa de bytes reais: manifesto, verificador, testes de verificador,
   OTA normal e sanitizado, scope e registrador adaptado. O wrapper sozinho não
   comprova builds ESP32 nem os gates pós-build.
5. Registrar as versões efetivas de runner/imagem, Python, compilador host,
   Arduino CLI, core e dependências. Frio e quente precisam de versões iguais;
   `ubuntu-latest` e setup-arduino-cli@v2 sem versão CLI não garantem isso sozinhos.
6. Executar frio e quente em ambientes novos e separados. Aquecer somente as
   dependências permitidas; nunca restaurar build, checkpoints, capturas,
   firmware ou outros resultados. Cada perna deve realmente executar os 157
   grupos. O mesmo run-id é seguro somente em fontes/workspaces novos separados.
   Verificar ausência de `RESUME skip` e de resultados preexistentes, fora deste
   adapter. Manter as mesmas opções, jobs, versões e plano em ambas as pernas.
7. Medir download/restauração/instalação, agregado, builds, OTA e upload
   separadamente. Não inferir ganho end-to-end de microbenchmark de instalação.
8. Promoção de artefato, quando simulada, deve consumir bytes da própria perna,
   conferir hash/identidade e repetir verificadores/OTA. Não executar nada do ZIP.
   Uma simulação bem-sucedida não libera publicação. O publisher não foi alterado.

## Testes executados

Vinte testes offline passam nos dois comandos:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 proposals/ci-acceleration/tests/test_benchmark_adapter.py
PYTHONDONTWRITEBYTECODE=1 python3 -O proposals/ci-acceleration/tests/test_benchmark_adapter.py
```

Cobertura: todos os vínculos de ambiente; fonte/ref/evento/repositório/run;
checkouts e ancestralidade com respostas git de fixture; pins e âncoras; raízes;
source inalterado; wrapper com exatamente duas mudanças; equivalência AST das
assertions e auxiliares; diff completo; tamper de arquivos/manifesto; tipos JSON;
symlinks/arquivos extras; rejeição de resultados preexistentes na largada;
aceitação dos resultados gerados depois; rejeição de execução otimizada; JSON duplicado.

As fixtures substituem explicitamente a leitura dos checkouts para testar o
materializador. Isso não acontece no runtime de produção. Testes não executam
os 157 grupos, ESP32, OTA real nem GitHub Actions. Não há resultado frio/quente.

## Limites e próximos gates de revisão

- O contexto Actions é conferido contra variáveis reais do processo, mas este
  protótipo não autentica API GitHub/OIDC, assinatura ou atestado de proveniência.
  Um processo local pode inventar variáveis e fixtures; isso nunca é uma run CI.
- O contrato não verifica acesso remoto ou autorização do usuário para push,
  execução, criação de cache, upload, promoção ou publicação.
- Materialização exige commit 6026 real. O checkout de trabalho desta preparação
  tem HEAD c8738eb e árvore staged 5ad62c; por isso não satisfaz esse requisito.
- Um workflow e seu teste de configuração independentes ainda precisam assegurar
  todos os passos, diretórios novos, caches estreitos, miss/hit efetivos, versões,
  logs e evidências pós-build. Este adapter não alega fazê-lo sozinho.
- Revisar os diffs e integrar somente como proposta isolada. Não substituir os
  scripts originais, afrouxar o gate do publisher ou considerar benchmark como
  aprovação para main/tag/release.
