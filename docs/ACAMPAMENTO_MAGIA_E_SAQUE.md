# Acampamento, magia e saque — 2026.10.07-camp1

## Como jogar

Na tela de exploração da região ou nas Ruínas, escolha **Acampar**. Marque se quer usar uma ração e o kit de dormir, depois confirme. O kit custa **80 ouro**, é reutilizável e pode ser comprado nos Suprimentos ou na preparação do acampamento. Ele aparece no sétimo espaço da bolsa e acrescenta **+2** ao teste; a ração é que determina a recuperação.

O teste usa D20 + sobrevivência + sorte (+2 com kit), contra dificuldade 11/13/15/17 conforme a região. Um 1 natural sempre provoca emboscada; um 20 natural sempre permite descansar. Sem ração, soma 50% dos máximos de HP e MP aos valores atuais, limitados ao máximo. Com uma ração, ambos ficam completos. Uma ração é reservada e consumida ao confirmar a tentativa, antes de rolar o dado.

Se houver emboscada, enfrente um inimigo da região e escolha **Descansar** no resultado da vitória. Fugir ou perder interrompe o acampamento, sem a cura e sem devolução da ração já usada. A animação dura 4,5 segundos e mostra a barra do tempo: sentado abatido sem comida; comendo carne com ração; saco de dormir ao lado quando o kit está selecionado. O fundo corresponde à região atual. A fogueira e as poses se alternam sem leituras do cartão durante o jogo.

## Magia e recompensas

O disparo do cajado agora é uma pequena bola de fogo com deslocamento e impacto. A técnica do mago usa uma animação ramificada de raio, diferente do ataque. Os efeitos foram preparados a partir de `bola de fogo.gif` e `raio 3.gif` da pasta conceito do usuário. Os efeitos continuam visuais: a regra de alcance da dungeon ainda é adjacente. Não anunciar ataque a distância de várias casas antes de implementar a etapa descrita em COMBATE_DUNGEON_PROXIMA_ETAPA.md.

O baú do chefe sorteia entre **150 ouro** e as três armas de segundo nível das outras classes. Armas já possuídas saem do sorteio, evitando duplicatas; quando as três já estão na bolsa, o resultado é ouro. O ouro respeita o teto de 999999. O item permanece em **Bolsa > Equipamentos**, com sua imagem e nome. Na cidade, selecione o equipamento e toque em **Vender**: a confirmação informa o valor, equivalente a metade do preço da região. Um equipamento usado deve ser retirado antes da venda. Transferência entre slots e troca entre jogadores ficam para uma etapa futura.

## Arte e persistência

Arte nova de acampamento gerada com a ferramenta imagegen integrada: quatro classes sentadas, duas poses sem comida e duas comendo; quatro quadros de fogueira e acessórios separados. Originais e PNGs convertidos: `assets/camp1`. Prompts completos: `assets/camp1/PROMPTS.json`. Conversor: `tools/prepare_camp_art.py`. Cabeçalho compilado: `firmware/RPG_POKET_2/CampArt.h` (404992 bytes de pixels em flash). Os fundos regionais existentes são reutilizados, sem alteração no pacote SD estradas1.

Save **10**, ainda com **96 bytes**, lê versões 1 a 10. Bytes reservados 90/91 guardam kit adquirido, escolhas, fase e dado do acampamento. O diário existente salva a rolagem antes de mostrar o resultado e registra cura, recompensa e venda uma única vez. Reabrir durante descanso repete a apresentação, não a recuperação já concluída. Não instalar um firmware antigo sobre saves que já foram gravados pela camp1: versões anteriores não entendem o formato 10. Sem mudanças em partições, namespaces, saves Heltec ou cartão físico.

## Validação

Versão candidata `cd416a0dd133d308339d6d39702952c9ebf068d3`: [CI37566935983](https://github.com/Oflodorporto/RPG-POKET-2/actions/runs/37566935983) passou as 16 suítes e compilação ESP32. Sketch2109198 bytes (67% de3MiB), variáveis globais53316 bytes (16%). Foram conferidas prévias do renderizador real, incluindo fundos regionais, três variações de acampamento, bola de fogo, raio, kit na bolsa e confirmação da venda. Testes cobrem 1280 combinações de acampamento, 1024 sorteios de baú, migração do save, falhas de gravação e repetição, venda e controlador real do sketch. A validação física desta versão será realizada pelo usuário.