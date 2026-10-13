RPG POKET • Diagnóstico separado de SD e Wi-Fi • SD-DIAG-2026.10.13-3
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

TESTE ATUAL W: ESCRITA LOCAL VERSUS DOWNLOAD, MESMA MONTAGEM
1. Grave a versão3 com as opções acima. Desligue o USB e ligue com o cartão.
2. Monitor Serial115200, “Nova linha”: envie W antes de qualquer outro teste.
   Não envie 1/P/D antes. W configura1MHz e mantém a tela APAGADA de propósito.
3. A cria w_local.pak em pasta diagnóstica nova: gera1MiB do padrão sintético,
   grava, fsync/close e relê com SHA completo, Wi-Fi desligado. Qualquer falha
   de escrita, fechamento, leitura ou condição encerra o teste antes de B.
4. Se A passar, informe SSID/senha só no Monitor local quando solicitado.
   B conecta com credenciais em RAM, confirma horário para HTTPS e recebe apenas
   o primeiro1MiB do fixture público para w_http.pak, outro arquivo exclusivo.
   O prefixo recebido também precisa passar pelo SHA esperado antes das leituras.
5. O escritor é fechado. B relê primeiro com o socket HTTP ainda aberto, depois
   fecha HTTP e relê com Wi-Fi associado, finalmente desliga Wi-Fi e relê.
   O cartão permanece na MESMA montagem durante A/B; não há remontagem oculta.
6. Aguarde W END; envie R e salve/envie o texto completo até END truncated=0.
   Não execute mais testes automaticamente. Depois de W só R/H são aceitos;
   qualquer repetição exige novo boot, mas aguarde a análise deste resultado.

W cria no máximo dois arquivos de1048576 bytes e não altera os antigos.
Não formata, não apaga, não renomeia artes ou saves. Os dois novos arquivos são
mantidos mesmo se incompletos; seus caminhos aparecem em NEW_DIRECTORY/NEW_FILE.
A primeira falha fica em first; prior_errors/prior_invalid indicam possível
influência sobre fases posteriores. Estados ausentes fazem a leitura SKIPPED;
uma mudança durante a leitura invalida a comparação mesmo que o SHA passe.
O limite é1MiB consumido pela aplicação na única transferência de B; o servidor
oferece6,7MB e buffers TCP podem receber bytes adicionais antes do fechamento.
Socket aberto indica o estado observado pela API, não tráfego contínuo garantido.
Não se lê com o escritor aberto, nem se acessam SD/LCD concorrentemente.
Os arquivos A/B ocupam posições diferentes no cartão; uma sequência isolada não
prova causa elétrica nem elimina falhas intermitentes. Se tudo passar, isso
não valida o instalador completo. O resultado será analisado antes de outro passo.
Cada leitura e a escrita local têm limite cooperativo180s; transferência240s,
inatividade20s, além dos limites de conexão/entrada. Chamadas do driver SD podem
levar tempo próprio. Não há um prazo rígido para todas as fases. Normalmente
cada leitura de1MiB leva poucos segundos, como observado em P; aguarde W END.

TESTE P PRESERVADO: PARCIAL EXISTENTE, SEM DOWNLOAD NEM ESCRITA
P não é necessário para W; as instruções abaixo preservam o teste anterior. O arquivo que já está no cartão é:
/RPGDIAG_18cb11d2f9db67a9/download.pak, 1048576 bytes;
SHA256: 7cdb705f9a9be23511a919cde2e8455f8abde9c8f9abed1de218ad54f8e4a554.
Não precisa copiar, renomear, apagar ou baixar nada.
1. Após gravar a versão2, desligue o USB por completo e ligue com o cartão.
2. Monitor Serial115200, “Nova linha”: envie P antes de qualquer outro teste.
3. A tela permanecerá APAGADA durante todo P. Isso é esperado para manter LCD
   fora da comparação. SPI usa1MHz. A primeira leitura ocorre com Wi-Fi desligado.
4. Quando solicitado, informe SSID e senha somente no Monitor local, nunca no chat.
5. Ele lê conectado, desliga Wi-Fi e lê novamente, na MESMA montagem do cartão.
   Não acessa GitHub/NTP nessa sequência nem cria arquivo. Uma última releitura
   ocorre após desmontar/remontar; ela aparece claramente como cycle2.
6. Aguarde P END, envie R e guarde o relatório. Depois de P, só R/H são aceitos;
   não é possível disparar D/S por engano. Antes de repetir P: novo cold boot.
O primeiro erro é preservado. prior_errors informa que um erro anterior pode
influenciar as fases seguintes. Desconexões de Wi-Fi durante a leitura aparecem
nos estados antes/depois; passagem de uma fase não prova a causa elétrica.
Fases sem o estado de rádio exigido ficam SKIPPED. Uma queda durante a faseON
invalida essa comparação, mesmo que o SHA passe. Confira valid/invalid_radio_phases.
As até quatro leituras são do mesmo1MiB. Cada leitura tem o limite de180s já existente;
montagem/conexão têm seus próprios tempos. Não aguarde novas imagens na tela.
Se a pasta tiver outro nome, somente PATH em ReadbackTarget.h é editável localmente;
o programa aceita apenas /RPGDIAG_<16 caracteres hex minúsculos>/download.pak.
Tamanho/hash continuam fixos para este parcial. Não coloque credenciais no header.

TESTES ANTERIORES PRESERVADOS (NÃO NECESSÁRIOS PARA P)
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
