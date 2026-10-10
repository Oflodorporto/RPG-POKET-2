# Pacote 6.2 — Cartas ao Entardecer

Versão 2026.10.10-cartas2, build 2026101002. Duas oportunidades por dia local, a partir das 09h e 18h. Uma carta pendente permanece até ser respondida, mesmo após reiniciar ou mudar o dia; uma missão aceita permanece até seu desfecho. Não é necessário manter o aparelho ligado nos horários. Se o jogador abrir tarde, pode responder à primeira carta e receber a segunda em seguida. Recusar não penaliza o herói nem troca a mesma oferta por outra. Relógio voltando não libera ofertas extras.

Esta entrega prepara o calendário e corrige a perda de cartas; o conteúdo ainda é o evento do hipogrifo. Os oito eventos com coleta, escolta e dungeon NÃO foram implementados nesta versão. Os doze contratos regionais da entrega anterior continuam disponíveis e são independentes das cartas. Próxima entrega: persistência própria e objetivos variados para os eventos, sem sobrescrever contratos.

Cartas da Guilda usa o pergaminho compartilhado e identifica primeira/segunda oportunidade. A confirmação explica exatamente qual carta será encerrada. Não há novas imagens nem leituras do cartão por quadro. As 488 artes são idênticas às da versão mundovivo1.

Save26 continua em128bytes e lê1..26. Byte97: bits0..1 mantêm o tier; bit2 é a oportunidade0/1. Bits3..7 são rejeitados. Saves≤25 continuam exigindo97≤3, e migram para oportunidade0 sem alterar estágio, dia, recompensa, contrato, bolsa ou acampamento. Não voltar a um firmware leitor25 depois de salvar26. Bytes90/91 e122/123, CRC124 e todos os IDs permanecem intactos. Nenhum save físico foi criado ou apagado.

Verificação prevista:39 suítes nativas, migração de todos os seis estados antigos, carta pendente após meia-noite, abertura tardia, segunda oferta, limite e relógio regressivo, pagamento único e escrita interrompida. CompilaçãoESP32-S3 e confirmação física são verificações separadas.

Teste na placa: com hora local válida e tutorial concluído, abrir Cartas; recusar ou concluir a primeira, conferir segunda após18h, reiniciar com carta pendente e confirmar que permanece. Receber uma recompensa uma vez. Confirmar contratos, camping, três slots e a surpresa da ilha preservados. Sem precisar mudar o relógio do aparelho; os casos de horários foram simulados nos testes de software.

Correção solicitada durante os testes: a tela de vitória, derrota e fuga agora mantém as recompensas no pergaminho. Removido o retângulo preto legado que escondia a tinta escura; XP e ouro em linhas separadas, com testes de contraste do fundo, limites de texto e ausência de alterações no save.
