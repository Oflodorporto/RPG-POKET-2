# Cooperativo de treino — candidata integrada local

14/10/2026. Base `6a23dda54114db611b818b51e4b1b942019098fa`. Branch local `local/coop-training-firmware`. Sem commit remoto, CI, instalação ou publicação. Não contém as alterações de comércio em lote do pacote 11.

## Escopo aprovado e implementado

O usuário aprovou começar pelo treino: duas placas, duas ondas e chefe, paisagem, turnos individuais, ataque/habilidade seguido de escolha de alvo, sem custos ou recompensas na campanha. Se alguém sair ou a conexão não puder ser recuperada, o treino termina sem alterar os personagens.

A candidata agora inclui hooks reais no sketch, Guilda e renderer, **desligados por padrão com `RPG_ENABLE_COOP_TRAINING=0`**. Uma qualificação separada deverá habilitar explicitamente a flag. Não basta copiar os arquivos para a release atual.

- Guilda: entrada própria “Coop treino”; PvP continua separado.
- Lobby: até quatro salas visíveis; escolha explícita pelo convidado e aceite/recusa pelo anfitrião. Perfis mostram classe, nível e aparência do personagem real.
- Admissão: somente o Game local é lido; a rede troca um perfil limitado de 27 bytes, nunca Game/save bruto. Perfis locais passam pela validação Save30 e pelo filtro de estado ocioso.
- Combate: host autoritativo, sequência/nonce/ACK/retry, snapshots e deduplicação. Cada jogador/inimigo tem sua vez. Apenas a execução automática dos inimigos espera 700 ms após a confirmação anterior para tornar as vezes visíveis; timers de transporte continuam ativos.
- Tela: renderer C++ real com artes existentes, dois heróis, inimigos/chefe, HP/MP, vez, onda, alvo válido, espera de confirmação e reconexão. Não cria segundo framebuffer nem novo pacote de assets.
- Habilidade permanece **Foco teste**, experimental, 2 MP. As quatro classes preservam atributos/aparência importados, mas poderes completos do combate solo não foram implementados. A interface informa essa limitação.
- Recursos são cópias RAM. Não há transferência de HP/MP/poções de volta, XP, ouro, itens ganhos, captura ou progresso de missão.
- Sair encerra, envia avisos por uma janela curta de 350 ms e devolve a tela/toque ao retrato. Não há migração de host ou retomada durável depois de reinício.

## Preservação e exclusão

Somente três arquivos existentes foram alterados: `RPG_POKET_2.ino`, `View.h` e `ClubRadio.h`. O restante da integração são arquivos novos. `Rules`, Save30/Persistence, protocolo/duelo PvP, assets/cartão e workflows publicados permanecem intactos.

A flag desligada preserva o caminho normal: nenhum menu cooperativo nem objeto de sessão/fila/radio cooperativo é criado. A entrada extra do enum Page foi anexada ao final, sem renumerar as páginas existentes.

A sessão detém uma licença exclusiva de rádio. PvP recusa a licença ocupada pelo treino antes de qualquer configuração/reserva; treino recusa PvP, atualização, busca/conexão Wi-Fi, campanha não ociosa e tela/touch indisponíveis. O loop do treino retorna antes das rotinas de campanha, cartas automáticas, relógio ocioso e updater. Existe também uma barreira no mesmo frame de entrada, depois do toque na Guilda.

O callback ESP-NOW apenas copia MAC, tamanho e bytes para uma fila FreeRTOS estática de oito posições. A fila vive durante todo o firmware. Tokens de geração descartam callbacks/datagramas atrasados. Session, regras, UI, sender e gestão de peers são exclusivos do loop. O cache de peers ESP-NOW é limitado a oito, preserva broadcast e o parceiro selecionado, e recicla entradas não vinculadas.

O toque é capturado em checkpoints de desenho, mas só executado no loop. Filas de 16 amostras, overflow, erro de leitura, alteração de estado/disponibilidade e troca de sala cancelam o gesto. Não existe callback de rádio executando gameplay nem alocação por frame.

## Protocolos e limites de rede

- PvP inalterado: magic `0x50434C31`, versão 5, 52 bytes.
- Admissão de treino: magic `0x414F4F43`, versão 1, modo `0x54`, até 96 bytes.
- Motor coop: magic `0x43504F43`, versão 1, modo `0x43`, limite 240; maior evento atual 227 bytes.
- Tokens frescos do host e do convite, MAC admitido, perfil exato, tamanho/tipo/checksum e estado de handshake são verificados antes de aceitar mensagens. Um boot usa entropia `esp_random`, nunca RNG da campanha.
- Descoberta: beacon de 700 ms, expiração de sala em 2,8 s, admissão limitada a 12 s.
- Motor pausa após aproximadamente 1,8 s sem peer. A sessão termina após 10 s sem atividade válida ou sem progresso de uma ação confiável pendente. Heartbeats saudáveis não encerram uma vez ociosa; heartbeats sozinhos não escondem um ACK permanentemente perdido.
- Resultados terminais permanecem visíveis; há janela limitada de 12 s para confirmar/retransmitir prova final. Pacotes antigos não reabrem a luta.

Esses mecanismos não são autenticação criptográfica nem proteção contra firmware adulterado/spoofing de MAC. ESP-NOW não criptografado é usado nesta candidata local. Não há promessa antifraude ou de integridade durável de campanha em rede.

## Wi-Fi e persistência

A pausa/restauração de Wi-Fi usa configuração temporária em RAM, não apaga credenciais e não chama `resumeConnection`, `connectNetwork`, Preferences ou journal. A conexão anterior é solicitada novamente sem resalvar perfil. Se a restauração falhar, a Guilda informa para reconectar Wi-Fi. Os controles/estado anteriores foram testados com fakes, inclusive falhas de inicialização e retorno.

O código foi conferido contra o core oficial 3.3.12: `_persistent` inicia true; a inicialização temporária usa false e restaura o padrão; disconnect sem erase preserva configuração; reconnect reaproveita a configuração. Fontes: [WiFiGeneric.cpp](https://github.com/espressif/arduino-esp32/blob/3.3.12/libraries/WiFi/src/WiFiGeneric.cpp), [WiFiSTA.cpp](https://github.com/espressif/arduino-esp32/blob/3.3.12/libraries/WiFi/src/WiFiSTA.cpp), [STA.cpp](https://github.com/espressif/arduino-esp32/blob/3.3.12/libraries/WiFi/src/STA.cpp).

Nenhuma escrita de configuração/app é observada nos fakes. Efeitos internos do driver Wi-Fi, condições assíncronas e gravações reais NVS ainda exigem ensaio físico. Não houve leitura dos saves reais do usuário.

## Evidências locais

- Admissão/sessão: 5.192 verificações otimizadas e ASan/UBSan, incluindo ondas/chefe, queda/reentrada, token/peer inválido, perfil adulterado, ACK perdido, atraso, final imutável, espera visual dos três inimigos e Save30/RNG idênticos.
- Rádio: 88 verificações de propriedade exclusiva, rollback, callback copy-only, cache/fila limitados e reentrada.
- Input/View: 176 verificações de controles reais, escolha de alvo, saída, troca de identidade de uma linha, erros/drag/overflow e retorno ao renderer em retrato.
- Wrapper real CoopDevice: 19 verificações de entrada/saída/reabertura e gates. O journal do teste não oferece API de escrita, demonstrando que a integração não depende dela.
- Renderer: 1.139 renderizações por build, sem alocações C/C++ durante desenho; 58 capturas, incluindo todas as classes/raças. Sanitizado e otimizado geram pixels idênticos. Herois comparados a `personalPixel`, inimigos ao decoder real, todos os 76.800 pixels mapeados, textos/controles/targets limitados à tela.
- Exclusão com ClubRadio real, contrato PvP v5/52 e binário default-off sem símbolos cooperativos verificados.
- Três regressões herdadas PvP otimizadas: protocolo, duelo e journal Save30, inclusive dez fronteiras de corte simulado.
- Os 19 arquivos herdados do motor/targeting/preview e toda a árvore anterior de protótipos permanecem byte a byte congelados.
- Revisão independente encontrou e ajudou a fechar: eventos rejeitados renovando liveness, esgotamento do cache de peers, licença liberada pelo objeto errado, fixture nativa sem artMemory e progresso confiável sem limite. Nenhum defeito de produção ficou aberto na revisão.

Sanitizadores: ASan e UBSan; LeakSanitizer desabilitado devido a ptrace do executor. Contadores de alocação continuam ativos. Estes são testes focados locais, não o agregado de release.

Medição nativa x86-64: Session 3.928 bytes, Input 440, Device 88, inbox + metadados da fila fake 2.040; soma desses objetos 6.496 bytes, além de pequenos globais. Fontes/hashes finais acompanham o pacote. Há static_asserts de orçamento para sessão ≤6 KiB, input ≤1 KiB, estado do device ≤256 bytes e inbox ≤3 KiB. Esses limites e `sizeof` x86-64 não são medição de heap/stack/PSRAM ESP32. Wi-Fi/driver possuem alocações próprias não representadas pelo contador C++ do modelo.

## Reproduzir e próximos gates

```sh
bash tools/test_coop_training.sh /tmp/coop-training-final
python3 tools/capture_coop_training.py --art /caminho/validado/artes.pak --out /tmp/coop-training-render
```

O pacote de artes precisa ser o atual, 666 assets/CRC `9b4bcfc0`. O `.gz` histórico rastreado não substitui esse arquivo.

1. Gate do responsável para abrir uma qualificação isolada, sem publicar e sem instalar toolchain neste executor.
2. Compilar o SHA exato no core 3.3.12 com `compiler.cpp.extra_flags=-DRPG_ENABLE_COOP_TRAINING=1`; conferir todos os TUs, tamanhos/partição, RAM interna/stack e build desativado. Nenhum firmware foi compilado nesta etapa.
3. Rodar regressão agregada apropriada e o [roteiro de duas placas](COOP_TRAINING_HARDWARE_CHECKLIST.md), incluindo restauração Wi-Fi, canto/soltura de toque, latência/FPS e coexistência com PvP/OTA.
4. Só então decidir sobre habilitação/publicação. O [mapa de controles](COOP_TRAINING_CONTROLS.md) e as capturas reais acompanham a candidata.

Não implementado: Save31, pets, recompensas duráveis, campanha cooperativa, terceiro jogador, poderes completos de classe, migração de host, equivalência plena ao solo ou balanceamento final. Nenhuma data de lançamento é prometida.
