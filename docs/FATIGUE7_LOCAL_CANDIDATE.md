# Pacote 7: debuff de cansaço, candidato local

Base exata: `3f9d3221b99e6b4ba55dc270a2a1d0a9155ea27b`, árvore
`34a32a69aa98ac7e8ba807a04ca065b438148dec`. Sem publicação ou alteração na placa.

## Auditoria e regra escolhida

O pacote 6 não aplicava penalidade de combate. As faixas de fadiga já existentes
são 0–39, 40–69, 70–89 e 90–100. A única penalidade anterior é −1 de sobrevivência
na próxima preparação de viagem a partir de 90; ela permanece igual.

O novo ajuste de PvE é estritamente acima de 70:

- 0–70: combate igual ao pacote 6.
- 71–89: −1 no dano final de cada ação ofensiva bem-sucedida.
- 90–100: −2 no dano final de cada ação ofensiva bem-sucedida.
- Uma única redução após dano, crítico, defesa, salvamento, bônus e dados de
  poderes, antes do limite de HP restante do alvo. Não se repete por raio,
  dardo ou golpe de um ataque múltiplo.
- Dano positivo tem piso 1; esquivas, raios que erraram e outros resultados de
  dano zero continuam em zero. Não há desconto em cura, buffs ou ações inimigas.

ATQ, DEF, HP/MP máximos, dados, chances e custos permanecem iguais. PvP usa seu
resolvedor próprio sem alteração. Ganho de fadiga, descanso, rações, recuperação,
viagens e Save30 não foram modificados. O desconto não sorteia nada. Uma vitória
adiada pode naturalmente adiar as rolagens de saque já existentes.

## Apresentação

O abrigo mostra o desconto por ação ou informa que até 70 não existe penalidade.
Durante a espera em combate, o topo mostra a fadiga atual e o desconto; durante
a animação, o botão Pular mantém seu espaço. O recibo real registra a conta
`Fadiga: max(1,dano-desconto)=resultado` e explica que vale uma vez por ação.
O total bruto é anterior ao desconto e o final/aplicado já o incorpora.

Recibos são históricos, capturados na resolução: mudar a fadiga depois não os
recalcula. Exploração (+4) e deslocamento real (+1) podem cruzar a faixa antes do
próximo ataque. Descansar ou recuperar até 70 remove o desconto no ataque seguinte.

## Validação reproduzível

`bash tools/test_fatigue7.sh` executa testes nativos e ASan/UBSan:

- 497.664 comparações de ações; 169.548 recibos reconstruídos independentemente;
  5.256 resultados de dano zero e 13.312 casos de limite de HP.
- 331.776 ações inimigas sem alteração; quatro classes, regras antigas/novas,
  níveis 1–20 e limiares 69, 70, 71, 89, 90 e 100.
- 1.024 travessias de limiar por exploração/movimento, recuperação e descanso.
- 16 fluxos do controlador com falha de save, retry, spam de toque e recarga.
- 1.145 quadros de UI/recibo, rótulos dentro da tela, sem mutação de Game/View/RNG.
- Suítes anteriores de fadiga, persistência, rádio/PvP, recibos e poderes.

O exemplo real de Maos flamejantes com semente 1 produz `4+2+4=10`, salvamento
reduz a 5; com fadiga 70 mantém 5, com 71 resulta em 4, e com 100 resulta em 3.

A comparação congelada do pacote 6 também pode ser repetida:

`python3 tools/test_combat_trace.py --baseline tests/fixtures/package6-procedural-only --save-format 30 --seeds 64 --report build/fatigue7/unchanged-combat.json`

São 2.497.818 registros sem alteração no corpus anterior de combate abaixo do
limiar; APIs normal, ponteiro nulo e recibo capturado concordam byte a byte.
O Game continua com 184 bytes e o Save30 com 160. Testes de integração completa
do pacote 7, build ESP32 e validação física são etapas separadas.
