# Compras e vendas em lote: candidato local

Base: `6a23dda54114db611b818b51e4b1b942019098fa`, com a apresentação e a história do README preservadas. O firmware publicado de referência é `0541056de5dc94a48b4d750ee0cb4c040df322b1`.

Este documento descreve um candidato ainda não publicado. Não há novo binário, release, atualização OTA ou validação física implícitos.

## O que muda

- Compras de rações, poções, mapas, talismãs, gazuas, cristais, kit e equipamentos usam um seletor comum com `- 1`, `+ 1`, `Máximo`, preço unitário, total, cancelar e confirmar.
- O máximo considera o ouro e o espaço disponível para aquele item. Kits e equipamentos continuam únicos, com quantidade máxima 1.
- Vendas no mercado, sucata e equipamento usam o mesmo padrão de quantidade, limitado também pelo teto de ouro. Equipamento vestido deve ser retirado primeiro; cristais e itens protegidos não se tornam vendáveis.
- Uma confirmação compra ou vende o lote inteiro. O recibo só aparece depois de o journal aceitar o salvamento. Repetir a confirmação não repete a transação.
- Rações passam de 9 para 10 em compras, coleta, recompensas, validação e viagens. Os demais limites, preços, recompensas em ouro e regras regionais permanecem iguais.
- O kit de dormir continua disponível no acampamento permitido de recuperação de escolta. Essa exceção não permite outras compras ou vendas durante a missão.
- Os botões reutilizam a arte medieval já presente na bolsa. As telas de comércio são visíveis imediatamente, sem aceitar uma confirmação escondida por animação de pergaminho.

## Salvamento e falhas

Mantidos Save30, leitura de Save29/30, 160 bytes, epoch 1 e os três personagens. Nenhum campo persistente, byte reservado, formato ou reset foi adicionado. Quantidade e orçamento da transação são estado temporário de interface.

Os testes simulam falha antes/depois da escrita, perda de confirmação, leitura falha, registro incompleto, desligamento e reabertura. O estado durável é o anterior ou o lote inteiro, nunca apenas ouro ou apenas estoque. Se o salvamento falha, a tela SaveError pausa o jogo com a transação pendente em RAM; tentar novamente apenas salva esse mesmo resultado. Reiniciar usa o estado completo durável disponível.

### Limitação de retorno ao firmware antigo

A atualização deste candidato lê os saves antigos sem zerar personagens. Isso não significa compatibilidade de downgrade depois de carregar 10 rações: o validador antigo rejeita um registro com 10. Se o outro lado do journal ainda contém um registro válido com até 9, o firmware antigo pode recuperar esse estado anterior; se ambos contêm 10, ele bloqueia. A prova com os headers exatos antigos verifica que essas leituras não escrevem nem apagam dados. Não recomendar downgrade nessas condições.

## Cobertura reproduzível

```sh
bash tools/test_bulk_commerce.sh normal
bash tools/test_bulk_commerce.sh sanitizers
python3 tools/capture_bulk_commerce.py
```

O gate local cobre 26 executáveis por modo e o leitor histórico congelado. A composição inclui:

- 24.154 casos de modelo/journal de comércio, quatro cidades/classes, quantidades inválidas e extremas, limites, cotações obsoletas, duplicatas, proteção, migração e três slots.
- 1.345 cenários do controlador real e 8.627 toques aceitos pelo TouchGate, incluindo caminhos de compra, cancelamento, recibo, toque mantido e recuperação de escolta.
- 1.858 estados visuais com limites de texto e saves imutáveis, incluindo cada quadro de abertura; chamadas new/malloc/calloc/realloc são proibidas durante a renderização testada.
- 6.925 verificações específicas de rações, 60 cortes de energia e 60 falhas/retries, além de testes existentes de exploração, viagens, acampamento, masmorras, persistência e PvP.
- Revisão independente: 10.782.720 estados de compra/venda em ASan/UBSan e verificação de todas as dez páginas de comércio no primeiro quadro. Duas regressões encontradas durante a revisão foram corrigidas e retestadas: kit na recuperação de escolta e confirmação invisível durante a animação.

ASan e UBSan são executados. LeakSanitizer não funciona no executor rastreado por ptrace e está desativado explicitamente; `RPG_DETECT_LEAKS=1` permite habilitá-lo em um ambiente compatível.

Os tamanhos no host são Game 184 bytes, ViewState 656 → 672 bytes, SaleQuote 8 bytes; o formato persistente continua 160 bytes. O pequeno aumento é estado estático da interface, sem novas alocações de heap/PSRAM. Não é uma medição de RAM livre no ESP32.

As imagens em `build/bulk-shop/screens` são capturas 240×320 do renderer real no host, não fotografias do aparelho. O contato de quatro telas é `compras-vendas-em-lote.png`.

## Gates restantes antes de publicar

- Compilar e linkar para o ESP32/Waveshare Touch LCD 2, conferir tamanho/partição e budgets.
- Criar a identidade e os gates de um novo pacote, preservando os gates congelados do pacote PvP publicado. Os gates imutáveis do pacote 10 não são redefinidos por este candidato.
- Qualificar o binário real e o caminho OTA e verificar toque/save no aparelho.
- Obter o gate de publicação. Nenhuma alteração remota foi feita por este trabalho.

Esta melhoria é independente de cooperação. Pode formar um pacote de qualidade de vida antes do co-op, sujeito aos gates acima; não requer esperar o protocolo co-op ou misturar mudanças maiores.
