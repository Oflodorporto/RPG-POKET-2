# Treino cooperativo — ensaio em duas placas

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
