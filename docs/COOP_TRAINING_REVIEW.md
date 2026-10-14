# Revisão técnica independente da candidata

Revisão read-only, 14/10/2026. Não houve publicação, instalação ou testes físicos.

Problemas identificados e corrigidos antes do congelamento:

1. Dois caminhos de evento rejeitado no Endpoint não incrementavam os contadores usados pela sessão para identificar rejeição. A chegada de eventos conflitantes podia renovar indevidamente o limite de conexão. Os caminhos de rejeição agora são contabilizados e a regressão cobre o prazo.
2. Heartbeats não podem esconder uma ação confiável sem ACK para sempre. A sessão tem limite separado de progresso de comando; uma vez saudável e ociosa continua aberta.
3. Uma instância de Device que não adquiriu a licença não pode liberar o rádio de outra. A licença também impede PvP/treino simultâneos.
4. O cache limitado de peers ESP-NOW recicla peers não vinculados, protegendo broadcast e parceiro atual. Convites de estranhos não preenchem permanentemente o cache até a próxima abertura.
5. Input separa captura e execução. A identidade completa da sala, além da revisão/hash/disponibilidade de combate, invalida gestos antigos. Uma janela cheia ou leitura inválida cancela o contato.
6. Cadência dos inimigos segura somente a resolução automática. Confirmação, retry e heartbeat permanecem ativos, e snapshots repetidos não estendem a espera.
7. Uma fixture nativa de retorno à Guilda precisava de artMemory válido. Corrigida no teste; as capturas dedicadas usam o pacote real validado.

Verificações independentes executadas: sessão (4.990 verificações na revisão, antes de 202 verificações adicionais da fixture final), Input (176), rádio (88), wrapper de dispositivo (19), exclusão com ClubRadio real, build default-off, gate de escopo, `diff --check` e árvore de protótipos congelada. A rodada final principal passou 5.192 verificações de sessão e os demais alvos novamente, otimizado e ASan/UBSan.

As capturas do renderer foram inspecionadas: alvos, rótulos, arte, telas de aceite, erro/reconexão e encerramento coerentes; nenhuma sobreposição evidente restante. Testes de pixels e limites acompanham o relatório principal.

Não ficou defeito de produção aberto na revisão. Isso não substitui a compilação completa ESP32, o orçamento real nem os ensaios em duas placas. Escalonamento entre núcleos, efeitos internos do driver Wi-Fi/NVS, orientação física, latência/FPS, memória e restauração real do rádio permanecem gates de hardware.
