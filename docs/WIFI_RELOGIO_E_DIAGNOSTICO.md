# Wi-Fi, relógio e diagnóstico — 2026.10.06-wifi1

Build 2026100604. Somente Waveshare SKU29667. Sem gravação automática, alteração de partições, formatação ou pacote `.rpg`. Mantém jogo, saves8 e pacote de 417 artes da estradas1.

## Redes

Configurações → Internet / Wi-Fi abre **Salvas**. Até cinco redes, com Anterior/Próxima e Conectar sem digitar novamente. Uma conexão bem-sucedida salva a senha; a antiga conexão única é importada. O último Wi-Fi usado volta automaticamente após reiniciar. Desligar desativa essa reconexão até conectar outra vez, mas mantém a lista. Esquecer pede confirmação e remove somente a rede escolhida. Lista cheia não substitui outra rede silenciosamente: esqueça uma antes de adicionar a sexta. Credenciais ficam na NVS da própria placa e não são enviadas ao GitHub.

Buscar faz varredura ativa assíncrona em todos os canais disponíveis de 2,4 GHz, com até 500 ms por canal. O código anterior já procurava todos os canais e não limitava os resultados a duas redes; esta entrega exibe canal e RSSI para diagnosticar o que aparece. SSID é apenas um nome: uma rede chamada “5 GHz” que aparece neste ESP32-S3 está sendo anunciada também em 2,4 GHz. Não é possível fazer esse chip enxergar uma transmissão exclusivamente em 5 GHz. Hotspot deve estar configurado em 2,4 GHz e anunciar seu nome.

O indicador verde varia de uma a quatro barras. Limiares de apresentação: 4 para RSSI ≥ −55 dBm; 3 ≥ −67; 2 ≥ −75; 1 abaixo disso. São faixas aproximadas da interface, não garantia de velocidade. RSSI é atualizado a cada dois segundos sem buscas, leituras de SD ou gravações periódicas na NVS.

## Relógio de descanso

Após 60 segundos sem toque, mostra hora e data em fundo preto. Ajusta a hora por SNTP ao conectar, no fuso de Brasília (UTC−3/BRT3). Sem hora confiável mostra traços e pede conexão, sem inventar uma data. A hora continua avançando após perder Wi-Fi, mas não é um relógio de calendário permanente depois de desligar a alimentação.

Brilho temporário de 10%, sem sobrescrever o brilho salvo. Primeiro toque restaura a tela anterior e o brilho configurado; não aciona nenhum botão. Atualização, diagnóstico, busca/conexão, viagem, animação de combate e turno automático impedem entrada no relógio. Os personagens e checkpoints não são alterados por essa tela. Como é LCD, a economia vem da redução da iluminação, não só do fundo preto.

## Continuar download

Configurações → Atualização. Quando há parcial com SHA correspondente ao pacote atual, aparece **Continuar**. Depois de reiniciar, Verificar versão inspeciona os parciais e oferece a mesma opção. Cada confirmação reaproveita blocos completos já salvos, com Range/Content-Range; há cinco tentativas automáticas. Troca de pacote, servidor que ignora Range ou falha na conferência exige recomeçar para não misturar arquivos. Não garantir retomada em qualquer servidor.

A tela mantém a porcentagem da interrupção e diferencia HTTP, encerramento da conexão, tempo esgotado, Wi-Fi desconectado, abertura/escrita no SD. Quando conectado, inclui o sinal em dBm. Não formatar cartão nem apagar saves para corrigir uma interrupção.

## Testar conexão

Atualização → **Testar conexão**. Três consultas HTTPS ao manifesto da Release do GitHub, incluindo redirecionamentos, certificado, relógio e leitura/parse da resposta. Mostra respostas corretas/tentativas, falhas, desconexões observadas do Wi-Fi, faixa de RSSI e tempo médio das tentativas (inclui requisições que falharam). Não baixa firmware/artes nem instala nada.

Isso mede falhas de requisições HTTPS, **não perda de pacotes ICMP**, nem todas as desconexões transitórias que ocorrem e se recuperam entre amostras. Três respostas corretas de um arquivo pequeno não comprovam estabilidade durante um download grande. Se falhar, enviar foto do resultado e da mensagem de download. Comparar a mesma placa com rede/hotspot 2,4 GHz perto da antena ajuda a separar causas. Ainda não há prova de que a falha relatada seja do roteador, servidor ou cartão.

## Estado e teste físico

Usuário gravou estradas1 pelo cabo. Vídeo mostra 48% → “Download pausado / pode retomar”; também relatou 16% e 22%. Não foi possível testar hotspot porque não apareceu na busca. Imagens mostram duas redes da casa; nomes não identificam frequência. GitHub confirmou resposta 206 para Range e arquivos da estradas1 acessíveis.

Esta nova versão precisa ser gravada pelo cabo pelo usuário para diagnosticar o problema sem depender do download antigo. Manter ESP32S3 Dev Module, Flash16MB, OPI PSRAM, partição app3M_fat9M_16MB e EraseFlash desativado. Depois: conectar Wi-Fi, testar conexão, verificar versão e reparar artes. Não retirar cartão. Teste real desta versão permanece separado da compilação e das simulações.

Referências: [Wi-Fi Arduino ESP32](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/wifi.html), [ESP32-S3](https://www.espressif.com/en/products/socs/esp32-s3). Validação e publicação finais registradas em `validacao/wifi1-*.log` e `validacao/wifi1-release-github.json` quando concluídas.
