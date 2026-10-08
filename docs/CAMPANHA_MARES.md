# Ato III — O porto das lembranças vendidas

Versão `2026.10.08-mares1`, build `2026100805`. Esta etapa torna jogável o ato de Mares. Aurora, Odran, a Vigília Perpétua e os finais continuam pendentes; não apresentar a campanha completa como concluída.

## Como jogar

1. Conclua a Cripta do Arconte. A conclusão anterior já conta: não é necessário repetir a dungeon, abrir um baú específico nem carregar uma relíquia aleatória.
2. Prepare um herói de nível 10 ou superior e viaje normalmente para Mares. O D20 e o deslocamento continuam ativos.
3. Abra **Conversar → Sabela Marébrava → Investigar a carga**. Confira a missão e toque em Começar.
4. Vença a escolta da Mão de Cinza. Avance as quatro cenas e confirme Registrar no diário. O contrato concede 120 ouro e 1.000 XP aos heróis da nova progressão, além do saque normal do combate.
5. Converse novamente com Sabela e escolha **Resgatar refugiados**. Vença o bloqueio, leia as cenas e registre o resultado: 180 ouro e 1.500 XP, além do combate. O objetivo passa a ser procurar Liora em Aurora.

Heróis legados recebem os valores de XP da escala antiga: 100/150. Todas as quatro classes e raças podem concluir o ato. Não exige filiação paga à Guilda: o resgate é uma urgência, conforme o cânone. Missões repetíveis da Guilda e evento do hipogrifo continuam sistemas separados.

## História e apresentação

A Carga de Cinzas apresenta os cristais com nomes riscados, o envolvimento da Coroa, a participação de Tomás e a confissão de Maelis. O Farol Apagado trata do resgate dos refugiados, do trabalho de Nilsa para reconectar a rota e de uma pista parcial de Anwen. O diário reconhece provas e resgate; Sabela e Liora respondem ao progresso.

Cada missão usa um encontro por turnos contra o Saqueador já existente. A guarda, Tomás e Nilsa conduzem o resgate e os reparos na sequência narrativa. Não há minijogo de escolta, mapa de cisternas ou navegação naval nesta entrega. As artes aprovadas de Mares e do Saqueador são reutilizadas; nenhum pacote visual novo nem acesso ao cartão por quadro.

## Continuidade e segurança

O jogo grava o encontro antes de entrar no combate e mantém a missão durante a batalha e a tela de resultado. Reiniciar retoma o combate ou reinicia a leitura do desfecho ainda não confirmado. Ler cenas não dá recompensas. A confirmação final entrega o contrato e grava as descobertas uma única vez; falha na gravação pausa o jogo, e tentar novamente grava o mesmo estado sem repetir o prêmio.

Derrota oferece recuperação com as penalidades normais. Fuga volta às pessoas de Mares. Nenhuma delas completa a missão, paga o contrato ou elimina os refugiados. Ambas permitem preparar o herói e tentar novamente. Vitória comum em Explorar não completa estas missões.

Save16 mantém 128 bytes e CRC no offset124. Byte120: provas/resgate, bits0/1. Byte121: missão ativa0/1/2. Bytes122/123 reservados. Importa formatos1–15 com esses campos zerados, preservando herói, conclusão de Cripta, loot e recursos. Não fazer downgrade depois de gravar save16 sem backup compatível. IDs de destinos, inimigos e páginas anteriores preservados; CampaignTask/Result anexadas ao enum.

Heltec preservada. Nenhuma placa gravada, NVS apagada, partição alterada, cartão formatado ou arquivo `.rpg` gerado pelo agente.

## Verificação

23 suítes: nova `campaign_port` verifica quatro classes × quatro raças, requisitos, consulta sem mutação, retomada, prêmio único, derrota/fuga, importação15, corrupção e isolamento dos encontros comuns. O controlador real verifica entrar/cancelar, falha de save ao iniciar/finalizar, cenas sem escrita, prêmio único e recuperação. O renderer verifica limites de texto e pureza dos desfechos, além das telas de Sabela e objetivo de Aurora.

Compilação ESP32 e publicação são registradas no arquivo de continuidade depois de concluídas. Teste físico desta etapa permanece separado da verificação no computador.
