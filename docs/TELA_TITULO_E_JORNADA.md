# Tela de título e jornada — titulo1

Versão **2026.10.07-titulo1**, build2026100705. Abertura vertical inspirada na referência do usuário: cenário original de pôr do sol, espada e brasão, botões grandes. Folhas ao vento, pontas da fogueira, brasas e hipogrifo distante se movem lentamente. O fundo de 240×320 e 256 cores ocupa 77.312bytes na flash. Nenhuma imagem é lida do SD durante a animação.

## Fluxo

- Todo reinício chega ao título; não executa turnos, viagens, descanso ou pagamento antes de Continuar.
- Continuar fica apagado e ignora toques quando não existe personagem válido. Usa o último slot escolhido quando válido; se ele estiver vazio/protegido, escolhe o primeiro válido. Saves recuperados também podem continuar. Leitura no título não grava personagens.
- Novo jogo entra na seleção de raça, classe e cores usando o primeiro slot vazio. Com os três ocupados, mostra a lista e exige excluir um explicitamente antes da criação. Cancelar ou falhar na exclusão preserva os personagens. Confirmação final da criação é o primeiro checkpoint do novo herói.
- Configurações no título reúne brilho, Wi-Fi, cartão, testes, horário e atualização. Voltar retorna ao título. Menu do jogo oferece Tela de título no lugar de Configurações. Os três slots continuam acessíveis por Menu > Personagens.
- O relógio após um minuto permanece disponível. Antes de Continuar, seu toque retorna ao título e não aceita/abre missões do personagem por trás da tela.

## Lore

**Menu > Diário > Objetivo** mostra Sua jornada: objetivo contextual, local e próximo NPC. Usa somente os campos já salvos do tutorial, Ruínas, Guardião e Cripta; não inventa missões concluídas nem concede prêmios. Pode consultar Pessoas e relembrar o prólogo. Nara comenta a classe e suas descobertas; Iria distingue Guardião do Limiar e Arconte Vaelor e encaminha as pistas para Mares. Atos posteriores continuam em preparo, indicados no jogo.

## Teste na placa

1. Atualizar por Wi-Fi, reiniciar e conferir título, animações e os três botões. Não precisa remover o cartão.
2. Continuar deve retomar exatamente o personagem e atividade já salvos; entrar em Configurações e voltar antes disso não deve avançar o combate.
3. Novo jogo: usar slot livre, testar cancelar em raça/classe, personalizar e confirmar. Não apagar um personagem só para testar o botão apagado; esse caso está nos testes automatizados.
4. Se todos os slots já estiverem ocupados, conferir a lista e cancelar a exclusão. Excluir somente se desejar realmente liberar um personagem.
5. No Diário, abrir Objetivo, conversar e relembrar. Ouro, itens e recompensas ficam intactos durante a leitura.

Save11 e leitura1–11 permanecem iguais; sem mudança de partição ou pacote de artes. Sem gravação automática na placa pelo agente, formato de SD/NVS, pacote .rpg ou alteração da Heltec. Compilação e testes em software não substituem teste físico.

## Validação final

18 conjuntos de testes e compilação ESP32-S3 aprovados. 21prévias geradas e amostra inspecionada. [CI37669986839](https://github.com/Oflodorporto/RPG-POKET-2/actions/runs/37669986839); commit `492d2d9664926b6ebdbf2e6693174d78564cb3f5`. Release/latest e arquivos baixados conferidos por SHA-256; pacoteSD inalterado. Teste físico pendente.

Firmware publicado: 2291840bytes; o log da compilação separa programa e variáveis. Brasão e fundo totalizam99.104bytes de flash; não ocupam novo espaço no SD.
