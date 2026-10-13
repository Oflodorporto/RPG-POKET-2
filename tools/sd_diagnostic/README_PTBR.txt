RPG POKET • Diagnóstico separado de SD e Wi-Fi • SD-DIAG-2026.10.13-1
Somente Waveshare ESP32-S3-Touch-LCD-2 (SKU29667).

ANTES DE GRAVAR
Este programa substitui temporariamente o jogo na placa. Use de preferência
a placa nova sem dados, indicada para este teste. O sketch não faz escritas
diretas em saves/NVS e mantém a configuração Wi-Fi só em RAM. Contudo, o core
Arduino pode inicializar/recriar a partição NVS antes de setup em certas falhas,
e uma configuração errada de gravação pode apagar dados. Não há garantia de
preservação absoluta. Em placa com dados importantes, faça backup da flash antes.
Não instala firmware por Wi-Fi.
Não formate nem repare o cartão para este primeiro teste: precisamos preservar
a situação que falhou. Não altere/apague RPGPOKET/artes.pak.

CONFIGURAÇÃO NO ARDUINO IDE
Espressif esp32 3.3.12; ESP32S3 Dev Module; Flash16MB, QIO; PSRAM OPI;
USB CDC On Boot Enabled; USB Mode Hardware CDC and JTAG;
Partition Scheme: 16M Flash (3MB APP/9.9MB FATFS);
Erase All Flash Before Sketch Upload: Disabled.
Core Debug Level: Warn (habilita mensagens de erro do driver SD).
Confirme a porta da placa. Não use uma configuração antiga de outra placa.
Abra SD_Diagnostico/SD_Diagnostico.ino. A biblioteca de tela já está incluída;
não instale outras bibliotecas. Grave somente após conferir estas opções.

PRIMEIRO TESTE, SEM INTERNET
1. Placa desligada, cartão com RPGPOKET/artes.pak encaixado. Ligue pelo USB.
2. Abra Monitor Serial, 115200, final de linha “Nova linha”. A tela inicialmente
   fica apagada de propósito: o primeiro teste precisa ocorrer antes da tela.
3. Digite 4 e envie. O teste usa 4MHz, lê o pacote inteiro antes de iniciar a
   tela, depois da inicialização e intercalando desenhos entre as leituras.
   Não há operações SD/LCD simultâneas. Depois testa uma remontagem separada.
   Cada leitura tem limite; espere STORAGE END. Observe se a tela mostra texto
   e barras vermelha/verde/azul. lcd.begin sozinho não confirma imagem correta.
4. Copie também o Serial de STORAGE BEGIN até STORAGE END: ele inclui mensagens
   do driver que não cabem no registro próprio da RAM. Digite R para repetir
   o relatório próprio retido na RAM. Copie o texto para um .txt
   no PC e envie. Não precisa salvar o relatório no cartão que pode estar falhando.
5. Para comparar 1MHz, primeiro copie o relatório; desligue completamente o USB,
   ligue novamente, reabra o Monitor e envie 1. Só um teste4/1 é aceito por boot.
   Compare boots separados. Uma falha anterior pode influenciar fases seguintes;
   o relatório registra isso. O comando não finge que a remontagem é a mesma fase.

O QUE É LIDO
O pacote existente permanece intocado. Esperado: 6661456 bytes;
SHA256: 5416f6d507601bc131751e89106332f291f8d1d09642faa264a9b61215dd43f1.
São registrados pedidos/retornos/errno do bloco final1344 e do setor final.
Um pedido de512 no último setor deve retornar336 bytes: ali isso é EOF normal.
No pacote completo, o pedido final1344 precisa retornar1344 e passar o SHA inteiro.
Pinos usados: SCLK39/MOSI38/MISO40, SDCS41, LCDCS45/DC42 e backlight1.
Sem reset GPIO adicional e sem testar outros pinos. Ler níveis de CS e observar
respostas SD/LCD não mede continuidade elétrica nem prova que a placa está perfeita.

TESTES OPCIONAIS DE REDE
N: baixar arquivo sintético do GitHub para hash em RAM, sem acessar o cartão.
   Funciona mesmo sem cartão. Pode ser feito antes de4/1; a tela seguirá apagada.
D: após4/1, baixar o mesmo arquivo para uma pasta diagnóstica nova no cartão.
O programa pergunta SSID e senha no Monitor Serial. Informe só localmente,
NUNCA no chat. A senha não é repetida nem incluída no relatório. Use Nova linha.
As credenciais ficam só em RAM; reinicie para descartá-las. Se a entrada exceder
o limite ou90 segundos, novos testes ficam bloqueados até reiniciar; R ainda
recupera o relatório. Isso impede que restos da senha sejam tratados como comandos. Rede2.4GHz necessária.
Evite fotografar a caixa onde acabou de digitar a senha. Não envie logs brutos
que porventura contenham dados pessoais; envie o trecho delimitado pelo comandoR.
Cada comandoN/D é aceito uma vez por boot. Cada um transfere cerca de6,7MB,
sem repetição automática. Usa HTTPS verificado e horário via NTP, sem conta/pagamento.
O arquivo sintético tem tamanho igual ao pacote para testar a mesma fronteira,
mas seu conteúdo e hash são diferentes. Não é instalável e não substitui as artes.
No comandoD, em1MiB: fsync/close, releitura e hash do trecho, reabertura append;
no final: fsync/close, releitura completa e SHA. Não lê com o mesmo arquivo aberto
para escrita. LCD pode desenhar entre operações. Primeiro erro fica no relatório.
Falhas deixam o arquivo diagnóstico parcial para análise; ele não é reutilizado.

RELATÓRIO NO CARTÃO (OPCIONAL)
S tenta criar um report.txt exclusivo em /RPGDIAG_<identificador aleatório>.
Se a escrita falhar, o relatórioR continua na RAM. Nenhum arquivo existente é
sobrescrito. No máximo uma tentativaS por boot. A confirmação de gravação fica
no Serial/RAM; naturalmente o arquivo já fechado não contém essa confirmação.
Relatório RAM é limitado a24KiB; R informa se truncou. Copie antes de desligar.
Pastas diagnósticas podem ser removidas manualmente depois de guardar os resultados.

VOLTAR AO JOGO
Regrave a versão oficial ota77 com as mesmas opções e Erase All Flash Disabled.
Não altere o cartão nem a tabela de partições. Esta ferramenta não reinstala o jogo.
Testes de computador/compilação não validam eletricamente o seu aparelho. O motivo
físico da falha ainda será determinado pelos resultados; não há correção presumida.
