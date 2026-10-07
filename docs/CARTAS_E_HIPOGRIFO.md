# Cartas de Maelis e o hipogrifo — cartas1

Versão 2026.10.07-cartas1, build2026100704. Primeiro evento completo da Guilda em Aeldra: **Asas sobre os telhados**. Disponível sem pagar filiação e sem sobrescrever contratos existentes.

## Testar na placa

1. Atualizar por Wi-Fi. Não é necessário remover o cartão: os quatro quadros novos do hipogrifo estão no firmware; as imagens já instaladas no SD permanecem iguais.
2. Com um personagem que já concluiu o guia de Nara, verificar a hora em Configurações > Horário. A carta do piloto fica disponível a partir de09h da data local. Pode acertar manualmente para testar, mas avançar a data impede novas ofertas em datas anteriores. Não é preciso manter a placa ligada às09h.
3. Em um estado tranquilo, abrir Menu > Cartas. Também pode aguardar o relógio de inatividade: um pergaminho pulsa acima da hora; tocar abre a carta, sem aceitar automaticamente.
4. Ler o pedido e recompensa. Fechar mantém a oferta; Recusar pede confirmação e encerra a oferta daquele dia. Aceitar exige HP positivo e nenhuma viagem, dungeon, acampamento, duelo ou combate em andamento.
5. A Guilda custeia uma passagem curta para o cenário da missão. Enfrentar o hipogrifo com ataques/técnicas/poções usuais. A ave é afugentada após a vitória. Receber e voltar confirma ouro/XP e retorno em uma única gravação verificada. Reiniciar durante o combate ou resultado retoma a missão; Menu > Cartas também permite retomar.

| Nível | Local e ameaça | Recompensa |
|---|---|---|
|3–4|Pomar de Carvalho, hipogrifo jovem|25ouro /20XP|
|5–9|Ruínas de Véspera, hipogrifo|70ouro /60XP|
|10–17|Costa de Mares, hipogrifo marcado|150ouro /140XP|
|18+|Telhados de Aurora, hipogrifo alfa|300ouro /280XP|

Os limites máximos de ouro/XP do jogo continuam valendo. A missão não concede drops extras nem progresso em contratos/Ruínas. A derrota mantém a penalidade normal de XP e o resgate com1HP; a fuga retorna com o HP/MP restantes. Nenhum recurso de viagem, cristal, alimentação ou taxa é consumido pela passagem. Subir de nível pela recompensa segue a regra normal de progressão.

## Persistência e compatibilidade

Save11 tem128bytes e CRC no byte124. Preserva todos os campos do Save10 até91, incluindo acampamento90/91. A extensão92–99 guarda data máxima, estado, faixa e retorno;100–123 estão reservados/validados como zero. Novas gravações continuam no mesmo diário duplo por slot, com verificação de leitura. Os três slots, exclusões protegidas e saves1–10 são preservados e lidos nos tamanhos originais64/96bytes. Abrir um save antigo não grava por si só; ações/eventos usam o formato novo. Firmware anterior a cartas1 não lê Save11; não fazer downgrade após novos checkpoints.

Aceitação, resultado, pagamento e retorno pertencem ao mesmo personagem e diário, sem transação separada que possa pagar duas vezes. Falha ao salvar pausa a tela e permite nova tentativa. Reinício antes de confirmar o pagamento retoma o resultado; após confirmar, o estado concluído impede repetir a recompensa.

## O que esta entrega ainda não faz

Este piloto oferece **uma missão por personagem/data**, depois das09h. Não anuncia duas ofertas distintas nem oito eventos, pois só este tipo está completo. Não acumula dias desligados; uma oferta antiga não aceita pode ser substituída pela data atual; missão aceita nunca expira à meia-noite. Voltar o relógio não reabre datas já processadas. Ajuste manual não é um sistema contra fraude deliberada.

Outros sete eventos, variedade diária, novos inimigos noturnos, demais regiões e campanha completa continuam planejados. O combate usa a mecânica atual; alcance em distância ainda é uma etapa separada.

## Arte

Hipogrifo original gerado para o projeto, olhando para a esquerda, com dois quadros de repouso/asas e dois de ataque. Original e conversões em assets/cartas1; conversor com metadados mantém a mesma escala e linha do chão. As artes do cartão e a Heltec não foram alteradas.
