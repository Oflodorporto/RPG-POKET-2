# Cooperativo: ponte local de treino

Estado em 14/10/2026: **candidata local, sem entrada no menu, sem inclusão no sketch e sem publicação**. Não é um firmware cooperativo pronto para instalar. Base: `dfc68c49f3264d4f2d3642266d44b82ca9b7c563`, que documenta o PvP publicado de `0541056`. Os 971 arquivos rastreados dessa base não foram modificados.

## Resultado desta etapa

Há uma fatia nativa de ponta a ponta que combina o motor e a escolha de alvo previamente congelados com três adaptadores novos:

- `CoopTraining.h`: lê dois `const rpg::Game` previamente admitidos, verifica `persistentValid` do Save30 e um estado ocioso conservador, e copia perfis para os atores da luta. Não retém referências à campanha nem oferece uma função para gravar resultados. HP, MP e poções usados ficam em RAM; não há XP, ouro, captura, progresso ou recompensa persistente.
- `CoopRadioPort.h`: porta compatível com as assinaturas ESP-NOW/FreeRTOS. O callback copia MAC real e bytes para uma fila estática de oito mensagens. Somente o loop decodifica, confere MAC/peer/nonce e chama o motor. A transmissão usa o tamanho serializado, nunca `sizeof(Packet)`. Falha de envio depende do retry/ACK do protocolo. A porta não inicializa rádio, cadastra peers, registra callback, troca Wi-Fi ou chama rotinas do PvP.
- `CoopPortraitBridge.h`: transforma pixels lógicos 320×240 no buffer existente 240×320, sem um segundo framebuffer e sem `setRotation`. A transformação inversa alimenta a seleção de alvo. O amostrador conserva a última coordenada válida ao soltar, exige 30 ms de liberação válida e cancela contato após falha de I²C/contagem/faixa.

O treino continua usando o conteúdo provisório do motor congelado: dois jogadores, duas ondas de três inimigos e um Guardião. Cada jogador/inimigo tem sua vez. Ataque → toque no alvo; Habilidade → Foco experimental → alvo. **Foco não é a implementação dos poderes de classe do jogo.** A ponte de pixels não é um renderer final com todas as artes, fontes e animações.

## Isolamento verificado

- PvP permanece magic `0x50434C31`, versão 5, pacote 52 bytes. Coop permanece magic `0x43504F43`, versão 1, modo `0x43`, serialização explícita; maior evento atual: 227 bytes, limite 240.
- `Training.start` exige MACs unicast distintos, nonce não zero, papel 0/1 e dois perfis válidos. O nonce e os perfis são entradas do harness, não um handshake implementado. A futura admissão precisa gerar um nonce novo e não reutilizá-lo após reinício. MAC e checksum não são autenticação criptográfica.
- Uma geração RAM por sessão e outra por ligação da porta impedem reaproveitar datagramas da fila após fechar/reabrir. Uma porta ainda ligada a um objeto `Training` reiniciado não entrega nem transmite até `detach`/`attach`.
- A fila e o objeto de rádio devem existir durante toda a vida do firmware. Inicializar a fila antes de habilitar callbacks. Não reinicializar/destruir a fila enquanto um callback puder executar. A fila FreeRTOS é responsável pela concorrência; os fakes nativos não simulam escalonamento entre núcleos.
- Toques são capturados, nunca executados durante checkpoints de desenho. A fila guarda 16 amostras; overflow cancela o gesto inteiro. O loop descarta toques com revisão, hash, disponibilidade ou geração de interação antiga. Pausa seguida de recuperação do mesmo estado também invalida o gesto anterior.
- A entrada de treino recusa luta, viagem, descanso, dungeon, campanha em andamento, missão de evento ativa, descoberta/baú, reserva PvP e efeitos transitórios. Histórico válido de dungeon, PvP já liquidado, missões comuns e poderes diários gastos permanecem intactos. Isso é um filtro técnico conservador de laboratório, não uma nova regra de produto.
- `stop` descarta apenas a sessão RAM local. Não envia desistência, não paga resultado, não recupera host reiniciado e não define o comportamento final de abandono do coop.

## O que a auditoria do firmware encontrou

1. `ClubRadio.h` é proprietário do callback ESP-NOW e canal 6, e suas entradas/saídas alcançam reservas, apostas e saves. Até `resumeConnection` pode gravar o perfil Wi-Fi. Nenhuma dessas rotinas foi reutilizada.
2. `FrameBuffer.h` fixa stride e limites em 240×320. Mudar apenas a rotação da classe base altera os limites lógicos sem alterar esses escritores. A candidata mantém o framebuffer em retrato e gira as primitivas: paisagem `(x,y)` → retrato `(239-y,x)`; toque retrato `(x,y)` → paisagem `(y,239-x)`. O sentido físico ainda precisa ser confirmado na placa.
3. `readTouch` não informa coordenadas quando retorna “sem contato”. O `TouchGate` atual gera borda de pressão, e não o ciclo completo necessário pelo coop. Além disso, `FrameBuffer.onChunk` chama `sampleTouch` durante desenho. Captura e despacho precisam permanecer separados.
4. `playerProfile` sozinho valida intervalos, mas não todo o inventário/estado do Game. A ponte usa `persistentValid`, não o validador legado `valid`, para preservar histórico legítimo do Save30.

A orientação oficial ESP-NOW confirma que callbacks executam na tarefa Wi-Fi e recomenda enfileirar trabalho; entrega MAC não substitui ACK da aplicação: https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/network/esp_now.html

## Ordem prevista para uma futura integração, ainda não executada

1. Autorizar a superfície de treino e suas regras antes de criar qualquer entrada no menu.
2. Implementar admissão de dois jogadores, identificação de papéis, troca validada de perfis e nonce novo. Não transmitir `Game`/save bruto.
3. Adicionar um coordenador exclusivo de rádio que recuse concorrência com PvP/OTA e preserve a conexão anterior sem writes inesperados. A porta desta candidata se conecta a esse coordenador; não substitui o PvP.
4. No loop, drenar rádio e atualizar o motor; depois despachar amostras de toque; sincronizar e desenhar um snapshot. Checkpoints do desenho apenas capturam novas amostras. Fazer o envio limitado pela porta. Ao sair, invalidar a porta antes de descartar o treino.
5. Construir o renderer completo usando a transformação comprovada ou outra estratégia revisada, sem alterar a produção em retrato.
6. Compilar para o core/placa reais, medir flash/heap/stack/PSRAM, e testar duas placas: quatro cantos do toque, envio/ACK perdido, afastamento/reaproximação, desligamento de cada aparelho e coexistência/exclusão com PvP e OTA.

Não há prazo de lançamento nem aprovação implícita para qualquer desses hooks.

## Decisões ainda necessárias

- Primeiro acesso será um treino explicitamente sem custos/recompensas, ou já um modo que afeta campanha?
- No modo definitivo, quais recursos entram/saem da sessão, quais recompensas existem e quem recebe? Um recibo durável sem duplicação exige projeto de persistência e aprovação separados.
- Desistência individual encerra a luta de todos, deixa o outro continuar ou substitui por IA? Existe penalidade?
- Se o host sair/reiniciar, a luta é encerrada ou deve sobreviver? O motor atual recupera somente o mesmo host ainda vivo; migração/retomada durável não existe.
- Quais poderes de classe entram na primeira versão jogável? O Foco experimental não pode ser apresentado como equivalência ao combate solo.

Esta etapa não implementa Save31, pets, campanha cooperativa persistente, pagamento durável, terceiro jogador, descoberta, host migration, menu, novo firmware ou instalação.

## Reproduzir os testes focados

Na raiz desta candidata:

```sh
prototypes/cooperative_platform/run_tests.sh /tmp/coop-platform-results
prototypes/cooperative/run_tests.sh /tmp/coop-platform-engine-regression
prototypes/cooperative_targeting/run_tests.sh /tmp/coop-platform-ui-regression
```

O script de plataforma compila dois executáveis de teste locais. Não inicia CI, rádio, Wi-Fi, upload ou instalação. ASan/UBSan habilitados; LeakSanitizer desabilitado pela limitação de ptrace do executor. O contador de alocações C++ permanece habilitado.

Resultados congelados:

- Plataforma: **230.981 verificações**, tanto otimizado como ASan/UBSan; zero alocações C++ instrumentadas. Os 76.800 pixels percorrem uma correspondência biunívoca e as bordas do buffer ficam intactas. Rede estável e com perdas/duplicações encerram com hash `230425349`. Save30 serializado de ambos os heróis e RNG originais ficam idênticos após vitória, desconexão/retomada e descarte local.
- Motor herdado: 141.759 verificações/201 cenários no otimizado, 141.525/200 com sanitizadores, mais revisão 18/18 em ambos. 96 seeds; 5.140 perdas e 3.065 duplicações.
- Toque herdado: 460.982 verificações no otimizado, 460.965 com sanitizadores, mais 16 casos/10.929 verificações da revisão em ambos.
- PvP publicado: três regressões otimizadas focadas passaram (`club_protocol`, `club_duel`, `pvp_reliability_save30`). Incluem dez fronteiras de corte simulado no journal de produção. Não é a suíte total nem uma nova qualificação de release.
- Revisão independente dos três headers novos: nenhum defeito funcional restante encontrado; os mesmos binários otimizados/sanitizados foram reexecutados com sucesso.

Tamanhos **nativos x86-64, não medição ESP32**: `Training` 3.480 bytes, `RadioPort` 2.088, total 5.568 por ponta; `ContactSampler` 12 já incluído no Training. O RadioPort nativo usa metadados da fila fake. Esses números não incluem stack transitória, a fila FreeRTOS real, renderer, assets ou memória livre. Não houve compilação ESP32, teste físico, RF, FPS ou medição de heap/stack de placa nesta etapa.

`FROZEN_INPUTS.json` registra os 19 arquivos herdados do protótipo e seus hashes, byte a byte inalterados. As únicas adições à base são fontes/relatórios sob `prototypes/`.
