# Validação da prévia inativa

Data: 14/10/2026. Base: `dfc68c49f3264d4f2d3642266d44b82ca9b7c563`.
Ambiente: cópia isolada, Python 3.12.14, biblioteca padrão, sem rede nos testes.

## Resultado local

- 30 métodos de teste passaram em modo normal.
- Os mesmos 30 passaram com Python `-O`.
- A suíte também executa o CLI nos dois modos e verifica que a saída mantém
  `publish_authorized: false`; tentativa com tag existente termina com erro.
- Vários métodos usam matrizes de mutações; 30 é o número de métodos, não o
  número de entradas adversariais nem o número de testes de produto executados.
- Nenhuma etapa das 143 tarefas de firmware foi executada novamente. A suíte
  apenas importa a definição do plano existente e gera logs/bytes sintéticos.
- Nenhum firmware ESP32 foi compilado, nenhum dispositivo foi atualizado e
  nenhuma run, tag, release, permissão, configuração ou pagamento foi alterado.

Comandos na raiz do repositório:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 proposals/qualified-artifact-promotion/tests/test_verify_candidate.py
PYTHONDONTWRITEBYTECODE=1 python3 -O proposals/qualified-artifact-promotion/tests/test_verify_candidate.py
```

## Cobertura adversarial

| Grupo | Casos cobertos |
|---|---|
| Procedência declarada | IDs de repositório/workflow/run/tentativa; fork; origem/evento/branch/SHA; conteúdo do workflow; execução falha/cancelada/incompleta |
| Evidência de fonte | commit/árvore/inventário divergentes; plano de comandos adulterado; checkpoint ausente/falho/skipped; log diferente |
| Sanitizers | ASan, UBSan, LSan e assertions desativados; plano com `detect_leaks=0`; quantidade/identidade das tarefas |
| Artefato | ID/nome/digest/tamanho diferente; candidato ausente/ambíguo; expiração; tentativa e repositório divergentes |
| Tempo e estado | snapshot vencido/futuro; eventos posteriores à coleta; qualificação antiga; main diferente; tag/release existente; consumo já registrado no snapshot |
| Bytes promovidos | firmware adulterado; manifesto com versão/build/schema/destino/hash/tamanho errado mesmo depois de recalcular seu hash externo |
| ZIP | travessia/absoluto/backslash/dot segment; nomes duplicados/extra/NUL; symlink/FIFO/executável/diretório; tamanho/expansão; CRC/DEFLATE inválido; prefixo/trailer; membro local oculto; ZIP64 local; comentários |
| JSON e CLI | campos ausentes/extra; chaves duplicadas; bool/int; NaN/Infinity/overflow `1e999`; float; JSON malformado; symlink local; preservação do ZIP; `-O` |

## Revisão independente

Uma revisão separada leu o desenho, inspecionou o código, executou a suíte e
construiu mutações adicionais. Identificou e reproduziu estas bordas, corrigidas
antes da entrega e cobertas por regressões:

1. Número JSON `1e999` escapava da rejeição de `NaN`/`Infinity`: floats agora são
   rejeitados pelo contrato.
2. DEFLATE corrompido podia produzir exceção de decodificador não normalizada:
   agora produz rejeição estruturada.
3. Observação podia alegar eventos posteriores ao horário de coleta: ordem
   temporal agora é obrigatória.
4. ZIP64 local e membro local oculto não eram detectados apenas pelo diretório
   central: cabeçalhos locais/cobertura contígua agora são verificados, com
   layout experimental deliberadamente restrito.
5. Comentário por membro não estava coberto pela proibição de comentários:
   agora também é rejeitado.

Os achados não permitiam publicação, pois o protótipo não tem publicador nem
saída que autorize publicar. Mesmo assim, as verificações foram fortalecidas
para manter documentação e implementação coerentes.

## Preservação e limites

O diff contém somente cinco novos arquivos sob
`proposals/qualified-artifact-promotion/`. Os workflows, firmware, assets,
README principal, testes/tools existentes e inventário do pacote 10 permanecem
idênticos à base. A branch da prévia não coincide com nenhum gatilho de push
configurado na base. Não foi feita alteração remota neste trabalho local.

O parecer vale para **uma prévia offline inativa**. Não equivale a aprovação para
merge em main, segurança de um publicador pronto, teste de API ao vivo,
qualificação de firmware, benchmark ou teste físico. As barreiras de ativação
listadas no README continuam abertas: autenticidade de aprovação/procedência,
consumo atômico/antirreplay, corrida main/tag, compatibilidade da cápsula real,
validação semântica/OTA e conferência de downloads/latest.

Parecer final da revisão: adequado nesse escopo, sem bloqueio restante para
entregar a prévia. Identidades revisadas:

- `verify_candidate.py`: `4e0be4be78f89b68f267567e52ff8724fa615649ac76653d4d3526301e516c23`
- `tests/test_verify_candidate.py`: `03154e5b766a823865cb2f70db883d32aa0cb451c38844259881e2a9b8a7382e`
