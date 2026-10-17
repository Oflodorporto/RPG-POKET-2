# Teste de rádio integrado à aba Wi-Fi

Versão candidata reconstruída: `2026.10.17-wifitest77`, build `2026101701`.
Esta implementação tem novos commits e hashes. Não reutiliza a qualificação do
candidato local perdido. A publicação depende da qualificação integral oficial.

## Como usar duas placas

1. Em ambas: Configurações → Wi-Fi → Testar → Rádio / 2 placas.
   O teste HTTPS existente continua disponível na mesma tela.
2. Aproxime as placas para o pareamento. Confira o MAC mostrado na outra tela,
   escolha somente a sua placa, envie o convite e aceite na outra. O pareamento
   próximo não é uma medida de distância. Convites expiram em 10 segundos;
   a busca mostra um resultado sem placa após 30 segundos e pode ser repetida.
3. Afaste para **2 palmos**, mantendo a mesma montagem e alimentação normal.
   Toque Continuar nas duas placas em até 5 segundos. O teste passa por Atual e
   N1–N4 automaticamente, com 30 pacotes por direção e intervalo de 700 ms.
4. Quando solicitado, ajuste para **1 metro**, depois **2 metros**, e confirme
   novamente nas duas placas. A distância é uma escolha manual, não é medida.
5. Se as placas não conseguirem coordenar na potência Atual, o relatório diz
   “não coordenado / sem medição”. Selecione a mesma distância e o mesmo nível
   nas duas antes de tocar Continuar. Esse nível é aplicado **antes** da nova
   tentativa de coordenação. É possível repetir outros níveis e distâncias.
6. Cancelar ou Sair encerra o teste e restaura o perfil original. Uma falha de
   restauração mantém o jogo pausado e oferece Tentar restaurar; os relatórios
   continuam disponíveis. Não considere a placa remota restaurada se não houver
   confirmação no relatório.

São solicitados N1=5, N2=11, N3=15 e N4=20 dBm (20/44/60/80 quartos de dBm).
O teto capturado do driver/país e o limite compilado podem reduzir o valor.
O relatório distingue solicitado, retorno da API e valor realmente lido.
**Atual preserva a configuração existente**, podendo já ser 20 dBm; os quatro
níveis não são quatro aumentos acima de Atual. O teto lido não mede a potência
RF emitida pela antena. PS, protocolo e canal são identificados e comparados.

O canal do diagnóstico é 6. Se desconectar de um AP alterar automaticamente o
país e impedir a restauração verificável, a entrada falha de forma explícita.
Pode-se usar Wi-Fi → Desligar e repetir o teste. Desligar conserva os
perfis e senhas, mas desativa a preferência normal de retomada de conexão;
**Esquecer** é outra operação e não é necessário para o teste. Perfis cujo teto
original não possa ser reconstruído exatamente sem mudar o país são recusados.

## O que significa o relatório

- A é a placa que gravou o relatório; B é o peer escolhido.
- RX/aceitos compara recepções únicas com envios aceitos pela API, por direção.
  Erros locais de envio são separados; não viram automaticamente perda de RF.
- Perda só é calculada com denominador não nulo, ambos os inícios confirmados,
  perfis/potências compatíveis, janelas de tempo válidas e resumo remoto válido.
  Resumo ausente, fila cheia ou atraso de processamento não significam 100% de
  perda. Os dados locais ainda observados são conservados.
- RSSI pertence ao receptor e à rodada identificada. RTT só existe quando um eco
  correspondente foi medido. O teste HTTPS não fornece esse RTT de rádio.
- Callbacks MAC são agregados de todos os tipos de pacote, sem correlação com os
  30 probes. Detalhes informa essa limitação; não são ACKs de aplicação.
- O resumo é trocado por prazo limitado na potência testada antes da restauração.
  Isso pode preservar a medida quando N1 funciona e Atual não. A medição e a
  confirmação posterior de limpeza são estados separados.
- Resumos recebidos depois de finalizar a rodada são ignorados; linhas concluídas
  são imutáveis. Cada tentativa tem sessão, rodada, etapa, distância e nível.
- A RAM conserva até 24 relatórios, inclusive ao sair/reabrir o teste. Desligar ou
  reiniciar apaga essa RAM. O anel descarta a linha mais antiga ao exceder 24.

CSV no SD é opcional. Cria um nome novo em `/RPGPOKET/radio-XXXXXXXX.csv`, verifica
bytes e CRC na releitura, e não sobrescreve relatórios anteriores nem formata o
cartão. Falha de montagem/gravação/verificação deixa a RAM intacta; um arquivo
parcial novo pode permanecer no cartão. “Cópia SD verificada” descreve a cópia
naquele momento; uma nova rodada remove esse aviso. As colunas `local_recorder_*`
identificam apenas esta placa, inclusive versão, reset e flags compiladas USB/PHY.
Essas flags não provam que USB esteja ativo ou que a potência tenha sido reduzida
neste boot. `-1` e campos de validade identificam informações desconhecidas.

## Segurança e validação

O teste usa protocolo/versionamento próprios, fila fixa e posse exclusiva do rádio;
não altera o protocolo cooperativo, Save30, os três personagens ou as partições.
Enquanto ativo, o fluxo de campanha, gravações e serviços de rede fica suspenso.
Não chama APIs de gravação de credenciais ou preferências do aplicativo. O SDK
pode executar sua calibração PHY normal ao iniciar o rádio; isso não é uma promessa
de ausência absoluta de escritas internas do fornecedor em NVS.

Simulações e capturas de tela são fixtures de software, não medições destas placas.
O firmware não afirma corrigir o alcance. A comparação física deve ser feita nas
duas placas, com a mesma caixa/montagem e alimentação de uso habitual.

Referência de quantização e país: [Espressif ESP-IDF 5.5.5, esp_wifi](https://docs.espressif.com/projects/esp-idf/en/v5.5.5/esp32s3/api-reference/network/esp_wifi.html).
