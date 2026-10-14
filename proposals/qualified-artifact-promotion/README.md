# Publicar os mesmos bytes que foram qualificados

**Status: proposta inativa + protótipo offline, sem autorização de publicação.**

Base estudada: `dfc68c49f3264d4f2d3642266d44b82ca9b7c563`, em 14/10/2026.
Este diretório não é um workflow e não deve ser copiado para `.github/workflows`.
Não altera firmware, testes de produto, inventário congelado, release existente,
permissões do repositório nem configurações de conta. Não foi executado CI remoto.

## Em palavras simples

Hoje o projeto passa pela bateria completa para aprovar um candidato e repete
quase todo o trabalho quando publica. A melhoria de maior impacto é guardar o
pacote que passou nos testes e publicar **esse mesmo pacote, byte por byte**,
quando sua publicação for autorizada. A qualificação continua completa. Antes de
publicar, ainda são conferidas a procedência do pacote, a autorização e a versão;
os testes OTA de host continuam sobre os bytes promovidos. Depois, os arquivos
baixados da release e o manifesto `latest` são conferidos novamente.

**Agora você não precisa configurar nem contratar nada.** A prévia pode ser
revisada numa branch isolada. Para ativar no futuro será necessário aprovar uma
nova versão/tag, revisar o novo fluxo e seu inventário e concluir os bloqueios de
segurança abaixo. O desenho usa o GitHub Actions existente; não pede PAT, novo
secret, runner pago nem ampliação das permissões atuais. O consumo continua
sujeito aos limites da conta. Não houve alteração de cobrança.

## Onde o tempo foi gasto

Os valores abaixo são diferenças entre `started_at` e `completed_at` da API dos
jobs/steps. São duas observações, não um benchmark controlado; fila e atualização
da interface podem explicar diferenças de segundos em outros resumos.

| Medida | Qualificação | Publicação |
|---|---:|---:|
| Job completo | 30m18s | 38m51s |
| Agregado completo | 25m24s | 32m22s |
| Instalar ESP32 | 51s | 56s |
| Compilar ESP32 | 1m04s | incluído abaixo |
| Compilar + conferir bytes + OTA | 3m40s, soma de duas etapas | 4m25s |
| Downloads públicos e latest | não publica | 48s |

Fontes: [qualificação 38580662456](https://github.com/Oflodorporto/RPG-POKET-2/actions/runs/38580662456/job/115909381500)
e [publicação 38587457500](https://github.com/Oflodorporto/RPG-POKET-2/actions/runs/38587457500/job/115933136646).
O recorte reproduzível está em `baseline-timings.json`.

O agregado representa aproximadamente 84% de cada job. Eliminar sua repetição
na publicação é a hipótese principal; o tempo final ainda precisa de medição
com autenticação, download e verificações reais. **Não prometemos uma duração ou
percentual de economia.** O pacote já usa três tarefas host em paralelo e quatro
threads de compilação ESP32. Aumentar esses números sem medir RAM/CPU pode piorar
o tempo ou causar falta de memória.

## Mesmo commit não garante o mesmo binário

As duas execuções usaram o commit
`0541056de5dc94a48b4d750ee0cb4c040df322b1`, mas os firmwares têm hashes distintos:

- Candidato qualificado: `8f19fb4e703721db0935d7a954f92a8da66ac4cc2fe451c6bbd0a276bd862ede`
- Firmware público: `a1e5eb0b08ab685b0f45f31ed773f07491e1960b853c3ec83fef468aa5a1b2ef`

Isso demonstra que a recompilação observada não foi byte-idêntica. Não identifica
a causa e não indica falha dos dois pacotes: ambos passaram por seus respectivos
gates de bytes/OTA. O desenho futuro deve promover arquivos por identidade exata,
sem recompilar, regenerar o manifesto, renomear uma versão antiga ou assumir
reprodutibilidade. A [release PvP já publicada](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.14-pvp77)
não será substituída por este candidato.

## Fluxo futuro proposto, ainda não implementado

1. **Qualificar numa branch explicitamente aprovada.** Conservar as 143 tarefas
   do plano completo, a preparação, modos normal/sanitizado, ASan, UBSan com
   interrupção no erro, LSan com `detect_leaks=1`, assertions, comparação de
   capturas, limites ESP32/PSRAM e inventário rígido. Não aceitar modo focused,
   tarefa omitida, checkpoint antigo ou resultado obtido com outro compilador,
   plano, ambiente ou fonte. Novos testes poderão aumentar a cobertura mediante
   revisão; nunca usar apenas o número 143 como prova de equivalência.
2. **Compilar uma única vez e qualificar os bytes finais.** Conservar core/FQBN,
   Save29→30 e Save30→30, todos os slots, linhagens históricas aprovadas, hashes e
   os testes reais de OTA host em ambos os modos. Arquivar os logs desses gates,
   incluindo o pós-compilação, hoje não integralmente contido no ZIP histórico.
3. **Selar uma cápsula de promoção separada das imagens de revisão.** Incluir
   firmware, arte, manifesto e `SHA256SUMS` exatos; identidade de fonte/árvore,
   inventário, plano e checkpoints completos, hashes dos logs e evidências dos
   gates. Registrar IDs numéricos do repositório, workflow, run e tentativa,
   branch, SHA, digest e tamanho do artefato. Upload imutável com nome que inclui
   SHA/run/tentativa; nenhuma sobrescrita. O ZIP histórico tem 109.076.215 bytes;
   não é entrada compatível com a cápsula compacta experimental de até 64 MiB.
   Tamanhos reais de todos os logs devem ser medidos; se não couberem, revisar o
   limite/formato, nunca truncar provas para fazê-las passar.
4. **Pedir autorização explícita para publicar a identidade exata.** Vincular a
   decisão ao commit/árvore, versão/build/tag nova, run/tentativa, artifact ID e
   digest, hashes dos três arquivos e prazo. Branch ou run com sucesso, nome do
   ator, push em main e um campo `approved: true` em JSON não são aprovação.
   Atualizar main apenas quando essa ação também estiver autorizada, com lease
   do SHA anterior e sem force push. Se qualquer identidade mudar, parar.
5. **Validar procedência com um adaptador confiável e somente leitura.** Buscar
   metadados completos/paginados da API oficial para a run e tentativa exatas,
   workflow e seu conteúdo aprovado, repositório de origem e cabeça, árvore,
   branch e único artifact ID. Rejeitar fork, `pull_request`, `workflow_run`
   herdado como autoridade, workflow de outro conteúdo, tentativa nova, run
   cancelada, artefato ausente/ambíguo/expirado, digest indisponível ou divergente.
   O relógio vem do processo confiável. Nunca ler expectativas de dentro do ZIP.
6. **Revalidar em ambiente efêmero com privilégio mínimo.** Verificador e scripts
   vêm da fonte aprovada, nunca do artefato. Não executar scripts/executáveis do
   ZIP nem restaurar cache binário. Materialização futura precisa de diretório
   novo, allowlist exata, arquivos regulares e defesa contra symlink/TOCTOU;
   este protótipo não extrai nada. Reexecutar o verificador semântico da nova
   versão e OTA host normal + ASan/UBSan/LSan sobre os mesmos bytes a publicar.
   Conferir hashes antes e depois. O job de testes não recebe `contents: write`.
7. **Consumir uma autorização de uso único e publicar sem rebuild.** Serializar
   promotores; validar main/árvore/tag e lease novamente no último ponto seguro;
   registrar consumo durável por repositório + versão + run + tentativa + digest.
   Criar tag nova para o commit exato e publicar só os três arquivos aprovados.
   O publicador usa apenas `contents: write` e `actions: read`, já presentes no
   fluxo atual, com credenciais efêmeras. Artefatos ou código não confiáveis não
   recebem esse token. Não aceitar sobrescrita/clobber, refresh automático da
   aprovação, troca de artefato ou rebuild como fallback silencioso.
8. **Conferir o que o usuário baixa.** Baixar os três arquivos da tag publicada,
   comparar tamanho/hash e bytes com a cápsula, verificar manifesto/destino e
   repetir OTA host sobre os downloads. Confirmar release/tag → commit e que
   `latest/download/manifest.json` é byte-idêntico. Falha significa publicação
   não verificada, não sucesso; não mover tag/latest ou apagar release sem nova
   autorização. Uma falha parcial exige reconciliação da transação original.

### Bloqueios obrigatórios antes da ativação

- Adaptador de API autenticado com paginação completa e normalização revisada.
  `completed_at` no protótipo é um campo normalizado a obter da tentativa/jobs;
  não se deve inventar que todo objeto de run tem esse campo. Conferir contrato
  de digest/tamanho do ZIP efetivamente retornado pela API com uma cápsula real.
- Aprovação autenticada externa ao ZIP e consumo durável/atômico da autorização.
  JSON local não autentica o usuário, não impede replay real e não consome lease.
- Protocolo de concorrência para a relação main/lease/tag/release. `concurrency`
  sozinho não impede que outro ator atualize main. Consultar main e depois
  publicar são chamadas distintas: a corrida entre elas permanece um bloqueio
  de ativação até existir coordenação comprovada com quem pode alterar main.
- Selador real da cápsula e verificação do conteúdo do checkout/inventário,
  toolchain, comandos, logs e gates. Um digest comprova identidade de bytes,
  não que uma afirmação dentro deles seja verdadeira.
- Verificador semântico da **nova** versão, OTA real-byte pré/pós-publicação e
  verificação de downloads/latest, todos integrados e testados. O protótipo não
  testa formato ESP, assinatura, TLS, CRC de arte, Save ou placa física.
- Revisão independente dos workflows finais, ações presas a SHAs completos,
  `persist-credentials: false` onde aplicável, testes de falha/retry/concorrência,
  nova qualificação de escopo e autorização separada para ativar/publicar.

Nenhuma dessas etapas está escondida atrás de um botão de habilitar. Não há
workflow pronto para ativação neste patch. Até esses bloqueios serem resolvidos,
o fluxo atual permanece sendo o único fluxo de publicação.

## O que o protótipo realmente verifica

`verify_candidate.py` usa apenas a biblioteca padrão de Python. Lê dois arquivos
locais, não acessa rede, não extrai arquivos, não executa comandos e não publica.
Recebe um ZIP e um contexto **fornecido pelo chamador**, separado em expectativas
revisadas e observações de API. Testa a consistência dessas afirmações:

- IDs e SHA/árvore/branch/workflow/run/tentativa; fonte e inventário em bytes;
- exatamente uma cápsula, digest SHA-256 e tamanho de transporte, hashes/tamanhos
  dos três arquivos, manifesto/destino e checksum inventory exatos;
- plano completo de comandos preso ao digest aprovado, 143 IDs únicos em ordem,
  cada checkpoint `passed`, comandos correspondentes e cada hash de log;
- afirmações explícitas de ESP32, escopo, OTA normal/sanitizado e sanitizers;
- observação de até cinco minutos, qualificação de até 72 horas, validade do
  artefato maior que dez minutos e ordem temporal consistente;
- main divergente, tag/release existente, run/tentativa já consumida no snapshot;
- ZIP com allowlist exata, sem duplicatas, traversal, nome absoluto, barra
  invertida, symlink, arquivo executável, diretório, criptografia, prefixo/trailer,
  ZIP64, comentário, cabeçalhos locais ocultos, campos extras ou data descriptors.
  O layout experimental exige CRC/tamanhos nos cabeçalhos locais e cobertura
  contígua até o diretório central. Compatibilidade com o ZIP real de Actions
  é um bloqueio de ativação, não foi presumida. Limites de tamanho, metadados
  e expansão são rígidos;
- JSON sem chaves duplicadas, floats, valores não finitos ou confusão bool/int.

O resultado mantém sempre `publish_authorized: false`,
`authenticated_provenance: false`, `lease_consumed: false` e
`real_byte_ota_executed_by_this_tool: false`. Código de saída zero significa
**somente consistência offline**. Não conecte esse resultado a um publicador.
Quem controla os dois arquivos pode fabricar afirmações coerentes; a suíte usa
justamente firmware/arte/logs sintéticos, claramente identificados, para testar
o contrato. Ela lê o plano host existente sem executar seus comandos.

### Rodar os testes baratos

Na raiz desta cópia de trabalho:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 proposals/qualified-artifact-promotion/tests/test_verify_candidate.py
PYTHONDONTWRITEBYTECODE=1 python3 -O proposals/qualified-artifact-promotion/tests/test_verify_candidate.py
```

A suíte inclui o CLI normal e otimizado. O `-O` não pode remover as validações:
o código de produção usa exceções explícitas, não `assert`. São testes do
protótipo, não nova execução das 143 tarefas do jogo nem uma medição de economia.

## Cache: melhoria secundária, a medir

O core levou menos de um minuto em cada observação, contra dezenas de minutos
do agregado. Portanto cache de dependências é secundário. Pode ser estudado em
patch separado para downloads verificados de Pillow/core/toolchain: chave exata
com SO/arquitetura, versão Python/Pillow, Arduino CLI, core, FQBN e configuração;
sem prefixos de restauração amplos nem cache gravado por origem não confiável.
Manter checksums/lockfiles e revalidar conteúdo restaurado. Medir hit, miss, tempo
de restauração/salvamento e tamanho antes de adotar. Nenhum cache foi adicionado.

Não compartilhar `build/`, executáveis host, objetos ESP32, checkpoints de teste,
resultados ou cápsulas por cache. Normal e sanitizer têm flags e semântica
incompatíveis. Cache não é prova de qualificação nem substituto de artefato
imutável. Não reduzir casos, seeds, imagens, LSan ou versões de referência para
obter um número de tempo melhor.

## Escopo e revisão

Todos os arquivos deste patch ficam neste diretório `proposals/`. A regra atual
do pacote 10 rejeita fontes adicionais fora do inventário congelado; portanto
**não se deve levar esta prévia a main e disparar a release antiga**. Não alterar
`check_package10_scope.py` ou seu inventário para acomodar a proposta. A próxima
versão deverá ter escopo/inventário próprios revisados e qualificação nova.
A branch de prévia não é a branch autorizada por nenhum gatilho atual.

A revisão independente e os resultados locais são registrados em `VALIDATION.md`.
Fontes de produto não foram modificadas. Nenhuma validação física é alegada.

## Referências oficiais

- [API de artefatos](https://docs.github.com/en/rest/actions/artifacts): IDs,
  metadados, digest, expiração e download; acesso de leitura a Actions.
- [Artefatos entre workflows](https://docs.github.com/en/actions/tutorials/store-and-share-data):
  artefatos v4 são imutáveis. Divergência de digest na ação de download pode gerar
  apenas aviso; o gate proposto deve falhar explicitamente.
- [Segurança em Actions](https://docs.github.com/en/actions/reference/security/secure-use):
  privilégios mínimos, pinning de ações e cautela com dados e execução não
  confiáveis. O nome de um workflow ou um token mais poderoso não prova confiança.
