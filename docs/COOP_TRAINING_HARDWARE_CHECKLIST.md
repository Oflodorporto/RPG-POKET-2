# Treino cooperativo — ensaio em duas placas

## Reteste coopdiag77 · pendente

Identidade publicada: `2026.10.15-coopdiag77` / build `2026101502`. Qualificação de software, publicação e downloads públicos verificados. Conferir a mesma versão nas duas placas e manter os três slots, a partição e as artes. Não apagar flash, saves ou credenciais.

O proprietário relatou descoberta/conexão somente a uma distância estimada de cerca de **12 cm** e falha mesmo lado a lado após informar que atualizou. As fotos de descoberta mostram o convidado sem sala listada longe e com **ENTRAR** ao aproximar; não mostram por si só o aceite do anfitrião. A alimentação foi descrita como independente. Não há causa física/RF comprovada nem ajuste de potência, sleep, canal ou LR nesta entrega.

### Primeiro teste de diagnóstico

- [ ] A cria sala; B procura. Confirmar versão local nas duas telas INFO. Manter alimentação e orientação; não alterar configurações de rádio.
- [ ] Quando a sala deixa de aparecer ou a conexão falha, fotografar **INFO nas duas placas**. Registrar distância aproximada, RSSI/idade, CH/RX, PS/TXmax, TX/RX PROBE/BEACON/JOIN/WAIT, API/MAC e ACK/retries, fila e retornos dos getters. Os números são locais; transmissão broadcast concluída não confirma recebimento pelo parceiro.
- [ ] Reaproximar uma vez e observar se os contadores e a listagem voltam. Só depois, se necessário, inverter anfitrião/convidado para distinguir o sentido afetado. Não repetir toda a campanha nem apagar saves para coletar esse diagnóstico.

### Recuperação e controles

- [ ] Em combate, conferir pontos animados durante **Reconectando...** e ausência de ações enquanto a sessão está pausada. Uma retomada dentro do prazo existente deve conservar revisão, recursos e resultado, sem gasto/ataque duplicado.
- [ ] **CANCELAR RECONEXAO** deve voltar ao lobby em paisagem, sem encontro/token/gesto antigo. Criar nova sala ou entrar como o papel oposto. Depois de um encerramento definitivo não há retomada automática da sessão anterior.
- [ ] Após LinkLost/timeout técnico, conferir **VOLTAR A SALA** central. Vitória, derrota e saída do parceiro mantêm **VOLTAR A GUILDA**; **SAIR** no topo continua retornando ao retrato com limpeza de 350 ms. Verificar toque, quatro cantos e reentrada.
- [ ] Abrir/fechar INFO com toque segurado e em mudança de estado; nenhum comando de combate pode atravessar o painel. Conferir funcionamento da rede com INFO aberto e legibilidade dos contadores extremos.
- [ ] Comparar campanha/Save30/RNG/poções/XP/ouro antes/depois. Conferir restauração de Wi-Fi/PvP, OTA física, SD, alimentação, heap/stack/PSRAM e tempo de quadro em ensaios separados.

Nenhum item acima está aprovado fisicamente. As barras indicam uma medição RX com idade, não alcance garantido, ping ou qualidade bidirecional. Sem amostra, campo desconhecido ou getter com erro não deve ser interpretado como valor medido válido.

## Registro histórico: reteste coopfix77

O trecho seguinte registra o estado após aquela publicação. As referências a “atual” e a retestes pendentes pertencem àquela etapa; os relatos posteriores e o ensaio vigente estão acima.

### Reteste da correção coopfix77 · pendente

Identidade publicada: `2026.10.15-coopfix77` / build `2026101501`; qualificação de software, publicação e downloads conferidos. Use a identidade e os hashes do [estado publicado](ESTADO_ATUAL.md). As duas placas devem ter o novo protocolo de convite v2; a compilação Arduino qualificada usa treino **ON por padrão**, sem override. As instruções antigas de flag OFF abaixo são histórico, não configuração de instalação desta correção.

Relatos anteriores de coop77: saída nas duas placas após toques repetidos, uma vitória em duas placas e posterior timeout de aproximadamente 12 segundos enquanto se aguardava aceite. O último caso foi reproduzido em software; os relatos não provam congelamento global permanente nem a causa física da perda de link. **Nenhum item abaixo está aprovado fisicamente para coopfix77.**

- [ ] Registrar versão/build em A/B, hash do binário, papéis host/convidado, distância, alimentação e Wi-Fi anterior. Preservar os três slots; não apagar flash nem resetar época.
- [ ] Repetir o aceite com A e B como anfitrião. Conferir contagem de 30 segundos no host e **PRAZO APROX.** no convidado; aceitar antes de vencer, inclusive perto do fim, sem renovação por pedidos repetidos. O prazo do host é a autoridade, mesmo se o convidado mostrar tempo restante.
- [ ] Não aceitar: conferir **CONVITE EXPIRADO**, ausência de início tardio e nova tentativa explícita. Recusar manualmente em outra tentativa: conferir **PEDIDO RECUSADO**. Distinguir desses casos o limite técnico de conexão inicial/sincronização de 12 segundos.
- [ ] Em combate com link saudável, permanecer mais de 30 segundos na escolha de habilidade/alvo; concluir a ação depois, sem vencimento humano nem gasto duplicado. Repetir com espera maior. Se ocorrer queda, registrar tela, momento e condições em vez de atribuí-la automaticamente ao prazo de seleção.
- [ ] Conferir toques rápidos completos, toques repetidos e contato segurado atravessando mudança de tela, sem comando duplicado ou saída involuntária. Nas telas finais, usar **VOLTAR A GUILDA** central, conferir retrato e toque nos quatro cantos; sair/reentrar em ambas as placas. O botão SAIR no combate também permanece no roteiro.
- [ ] Repetir uma partida completa de duas ondas e chefe e os cenários de derrota, saída e interrupção de link do roteiro abaixo, sem afirmar alcance ou correção de RF com base na simulação.
- [ ] Comparar campanha, HP/MP, XP/ouro/itens, saves/época, RNG e artes antes/depois; sem recompensas ou alterações de campanha pelo treino. Conferir restauração Wi-Fi/PvP, três slots e atualização física, sem modificar potência/canal/sleep como parte deste teste.

Os ensaios de RF/interferência, toque/orientação, energia, SD, USB/OTA, heap/stack/PSRAM, latência e FPS permanecem pendentes. Guardar observações/fotos/logs com a identidade exata, marcando cada cenário como aprovado, falhou ou não executado. Capturas host e CI não substituem esses resultados.

## Registro histórico de preparação inicial

O texto a seguir é preservado como registro da candidata anterior, inclusive sua flag OFF e a ausência de binário qualificado naquela etapa. Para o reteste atual, use a identidade, ON padrão e limites documentados acima; os cenários físicos ainda são úteis.

Este roteiro ainda não foi executado. A candidata local tem `RPG_ENABLE_COOP_TRAINING=0` por padrão. Uma futura compilação de qualificação deverá habilitar explicitamente a flag; não instalar como atualização pública nem misturar com comércio em lote.

## Preparação

- Duas Waveshare SKU29667 com o core ESP32 3.3.12, LCD/touch funcionais e pacote de 666 artes CRC `9b4bcfc0` previamente válido.
- Escolher um personagem ocioso em cada placa. Registrar HP, MP, poções, ouro, XP, nível, fadiga e progresso antes do teste. Confirmar que campanha/dungeon/viagem estão fora de execução.
- Separar o backup habitual dos três slots, sem apagar flash, resetar época ou instalar formatos de save novos. Não usar personagens sem backup para a primeira prova física.
- Anotar hash/identidade do binário qualificado, versão, aparelho A/B, hora e estado Wi-Fi anterior. Não há binário qualificado produzido nesta etapa.
- Registrar flash/partição, heap interno mínimo/livre, stack das tarefas loop/Wi-Fi e PSRAM livre antes, durante e depois. O código não cria um segundo framebuffer nem novos assets.

## Fluxo básico

1. Na Guilda, tocar em “Coop treino”. Confirmar que somente esse modo fica em paisagem; a Guilda continua em retrato. O mesmo dedo usado para entrar não deve acionar um botão da nova tela.
2. A cria sala. B escolhe explicitamente a sala identificada pelo código mostrado. A confere classe/nível/aparência de B e aceita. Antes do aceite, nenhum turno começa.
3. Repetir invertendo A/B como host. Havendo mais salas de teste, conferir que cada uma das quatro linhas seleciona o host correto. Sala expirada/ocupada não pode receber o toque antigo.
4. Conferir raça, roupa, classe, nível e HP/MP. O modo mostra “TREINO” e “sem XP/ouro/sem gastos na campanha”. Habilidade é “Foco teste”; não representa todos os poderes de classe.
5. Ataque → tocar em cada inimigo. Habilidade → Foco teste → inimigo. Cancelar antes do alvo não deve gastar recursos, rolar dano ou trocar turno. Segurar, arrastar, sair da área e voltar não deve disparar.
6. Defender e poções só alteram as cópias do encontro. Conferir bloqueios de recurso, espera de ACK e turno do outro jogador. Cada inimigo deve ter sua própria vez visível, sem pular todos entre dois frames.
7. Completar duas ondas e o Guardião. Conferir que os dois aparelhos encerram com o mesmo resultado. Repetir com derrota. Não há recompensa ou atualização de missão/campanha.
8. Sair em lobby, confirmação, durante turno local, turno remoto, espera de ACK e resultado. A outra placa deve encerrar por saída recebida ou, se o aviso se perder, por conexão irrecuperável. Nenhuma saída fabrica uma vitória.
9. Ao voltar, conferir retrato e os quatro cantos do toque, sem liberação fantasma. Reabrir e jogar novamente; ações, sala, alvos e avisos antigos não reaparecem.

## Quedas e recuperação

- Afastar/reaproximar as placas: pausa de transporte após cerca de 1,8 s; recuperação do mesmo host antes do limite deve manter revisão/recursos sem ataque duplicado.
- Interromper o link por mais de 10 s: treino termina. Heartbeats sem confirmação de um comando também não podem deixar a sessão presa indefinidamente.
- Desligar o cliente; desligar o host. Não existe migração de host nem retomada durável. Reconectar aparelho reiniciado não pode assumir a sessão anterior.
- Fazer o teste perto do término de onda e do golpe final. ACK perdido não deve repetir dano, gastar outra poção ou conceder recompensa.
- Induzir falha de leitura do touch somente por método seguro de bancada. O contato deve ser cancelado; a próxima ação exige soltar e pressionar novamente.

## Rádio e campanha

- Não permitir entrada simultânea com PvP, atualização, busca ou conexão Wi-Fi em andamento.
- Sair do treino e abrir o PvP existente. Conferir descoberta, convite e duelo normais. Sair do PvP e abrir treino. Os modos não compartilham protocolo/pacote.
- Ensaiar entrada com Wi-Fi desligado e com Wi-Fi previamente conectado. Ao sair, conferir modo/reconexão anteriores e ausência de senha apagada ou gravação nova de preferências do aplicativo.
- Antes/depois, comparar os três saves, época e artes byte a byte. O código do treino não chama persistência; efeitos internos do driver Wi-Fi só podem ser confirmados nesta etapa física.
- Deixar uma carta automática prestes a aparecer. Enquanto o treino estiver aberto, ela não deve interromper a tela nem disparar alteração de campanha. Depois de sair, o fluxo normal pode voltar.

## Critério para avançar

Não considerar aprovado se houver reset, perda de input, desincronização, retorno com personagem alterado, erro de memória, falha de restauração do rádio ou operação simultânea com PvP/OTA. Guardar passos, logs, fotos e identidade do binário. A compilação ESP32, orçamento real e estes ensaios permanecem gates obrigatórios de qualificação, separados da publicação.
