# Cooperativo local: paisagem e escolha de alvo por toque

Protótipo local de 14/10/2026. Tela lógica de 320 × 240. Não instalado, integrado ou publicado. Esta etapa implementa o controlador de interação solicitado: escolher Ataque ou Habilidade, depois tocar no inimigo. É exclusivo do protótipo cooperativo. Retrato, PvP, campanha, firmware, saves, assets, rádio e as provas anteriores permanecem inalterados.

## O que foi implementado

`CoopTouchUi.h` adapta o `Endpoint` já congelado. `onTouch(endpoint, pressed, x, y, now)` recebe coordenadas lógicas de paisagem. Os estados são escolha da ação, escolha da habilidade, escolha do item, escolha do alvo e comando pendente.

- Ataque → toque em inimigo vivo → envio de uma intenção. Há contorno visível nos alvos elegíveis.
- Habilidade → Foco* → toque no inimigo. **Foco é exclusivamente a ação experimental existente no motor, 2 MP e +3 dano; não representa os poderes de classe completos.** Não foram implementados novos poderes.
- Defender envia a defesa já existente. Itens abre as poções HP/MP já existentes, aplicadas somente ao próprio jogador. Cura/suporte em aliado é possibilidade de projeto futuro, não função entregue.
- Cancelar está disponível antes do envio, em um botão de 75 × 30. Volta à escolha da ação sem pacote, gasto, rolagem ou avanço de turno. Não há confirmação adicional após tocar em um alvo válido.
- O dedo precisa pressionar e soltar na mesma região. Segurar, repetir amostras, soltar sem pressão anterior, arrastar entre botões/alvos ou soltar fora não dispara outra ação.
- O controlador só aceita escolhas na vez do jogador local e com o endpoint disponível. Foco e poções são bloqueados se o recurso/estado exigido não estiver disponível.
- Mudanças de sessão, revisão, turno, onda ou estado confirmado invalidam a seleção e qualquer gesto armado. Um slot reutilizado por um inimigo de outra onda não herda o toque anterior. Alvo morto, ausente, aliado ou espaço vazio não envia ataque.
- Durante envio/espera de confirmação, os controles e Cancelar ficam bloqueados. O cliente não diminui HP/MP nem resolve dano especulativamente. No host, o custo ocorre quando ele aceita e resolve a ação; aguardar ACK depois disso não gasta novamente.
- Rejeição do host volta à escolha da ação; não adapta a intenção antiga para outro turno. Retry, sequência, nonce, ACK e deduplicação continuam pertencendo ao motor original.

A elegibilidade usa `coop::legal`, a mesma função autoritativa usada pelo host. Não há fórmula paralela de custo/dano nem regra nova de alcance. Este motor local não tem alcance espacial; todos os inimigos vivos permitidos pelo modelo são selecionáveis. O hitbox visual não altera a regra de combate. Chamadas malformadas que contornem a UI ainda são validadas pelo host.

## Contrato de uso

Chamar `sync(endpoint)` depois de receber mensagens e antes de desenhar. `onTouch` também chama `sync`. A integração futura deve serializar a entrega de eventos do transporte e do toque: estes objetos locais não implementam acesso concorrente nem ISR/callbacks de rádio.

O parâmetro `pressed` representa o estado de **um contato**, não um evento de clique. Reportar perda/cancelamento de contato como release fora da tela, por exemplo `onTouch(endpoint, false, -1, -1, now)`. Não simular liberação sobre um alvo depois de uma falha do touch. Multi-touch não é parte desta interface.

`playable`, `eligible`, `actionError` e `hitTest` são consultas. `targetRect` devolve retângulo vazio para slot fora do intervalo e para slots vazios do chefe. Chamar `sync` antes de consultar os estados de interação; o renderer recebe um snapshot emitido após essa chamada.

Os IDs de instância do motor não são índices de slot: inimigos da onda 1 usam IDs 10/11/12 nos slots 3/4/5. O segundo inimigo do exemplo é o Lobo (ID 11, slot 4).

## Geometria de toque

Retângulos usam limites semiabertos: origem inclusiva, limite final exclusivo.

- Botões de ação: (4,199,75,30), (83,199,75,30), (162,199,75,30), (241,199,75,30).
- Inimigos comuns, com área aumentada em torno do sprite: (185,45,61,50), (252,70,63,53), (185,98,61,50).
- Chefe: (202,50,104,98). Slots 4/5 ficam vazios.

Não há sobreposição entre hitboxes. As áreas comuns são de 61 × 50 ou 63 × 53, e a do chefe é de 104 × 98. Essas são propostas geométricas no LCD, não certificação de conforto físico. A altura de 30 pixels dos botões exige teste na placa.

## Orientação física: ainda não implementada

A UI já trabalha em 320 × 240 lógico. Não se chamou `setRotation`, não se mudou driver/calibração, e não se rotacionou o restante do jogo.

Antes de integrar, conferir qual orientação/calibração o driver retorna e transformar exatamente uma vez. Se, e somente se, o touch vier em coordenadas de um retrato 240 × 320 já calibrado, as duas conversões candidatas são:

- 90° horário: x_lógico = 319 − y_retrato; y_lógico = x_retrato.
- 90° anti-horário: x_lógico = y_retrato; y_lógico = 239 − x_retrato.

O sentido correto depende do painel/driver. Medir nos quatro cantos e no centro, testar release/arrasto nas bordas e comparar com os hitboxes antes de habilitar a tela real. Se o driver já fornece paisagem, nenhuma segunda rotação deve ser aplicada. Entrada bruta fora da faixa deve ser rejeitada, não limitada artificialmente sobre um botão.

## Verificação e limites

A suíte nova cobre seleção/cancelamento, custo após confirmação do host, recursos insuficientes, poções próprias, toque repetido e arrastado, release antecipado, bloqueio fora do turno, snapshots alterados durante o gesto, novo nonce, morte do alvo, mudança de onda, slots vazios, host recusando alvo/turno/recurso inválidos, rejeição stale, perda de comando, perda de ACK e deduplicação sem nova rolagem. Classifica exaustivamente os 76.800 pixels em três estados, total de 230.400 posições.

Resultado final da suíte de UI: 460.982 checks no build otimizado com emissão de trace; 460.965 checks no ASan+UBSan sem emissão de trace; sete grupos de cenários em ambos. A revisão independente acrescenta 16 casos e 10.929 checks em cada build, inclusive 2.000 combinações de press/move/release. Dois defeitos foram corrigidos antes do congelamento: arrastar para fora e voltar ao mesmo alvo não pode disparar; erro de um comando alheio não deve substituir a confirmação da sequência local aceita.

Os testes do motor original são reexecutados sem alterações, inclusive perda/duplicação/reordenação em 96 seeds e as 18 regressões independentes originais. O terminal do trace de combate original continua revisão 27, hash 180348654.

A medição g++ nativa é `sizeof(TouchUi) = 48`, `sizeof(Endpoint) = 2800`. Dois controladores acrescentariam 96 bytes aos 5600 bytes dos dois endpoints, excluindo stack, framebuffer, arte e harness. Nenhuma alocação C++ de heap foi observada no teste instrumentado. Isso não é medição de RAM, PSRAM, flash, desempenho ou stack de ESP32. Um framebuffer RGB565 de 320 × 240 representa 153.600 bytes, sem afirmar que foi alocado no firmware.

O renderer Python/Pillow é só ferramenta de geração de prévias no computador, usa memória dinâmica e não integra o produto. O GIF usa tempos ilustrativos do roteiro; não representa latência ou FPS de placa. O cenário de chefe final é uma fixture separada e identificada, não uma progressão instantânea da primeira onda.

LeakSanitizer fica desabilitado por limitação de ptrace do executor; AddressSanitizer e UndefinedBehaviorSanitizer são executados. Não foram executadas as suítes totais do jogo, ESP32, rádio, duas placas, calibração, rotação física ou teste de dedo real.

## Reproduzir

Aplicar somente os arquivos novos `prototypes/cooperative_targeting` sobre o checkout base que já contenha o protótipo cooperativo congelado. Rodar:

    prototypes/cooperative_targeting/run_tests.sh /tmp/coop-ui-targeting
    prototypes/cooperative/run_tests.sh /tmp/coop-ui-engine-regression

Para as prévias, manter as fontes dos assets `clock9-release-work` e a estrutura de workspace descrita no renderer/manifesto. O ZIP não duplica o atlas. As imagens prontas abrem sem fontes do jogo. A UI é uma implementação C++ nativa exercitada por testes, sem touchscreen conectado ou aplicação interativa distribuída nesta etapa.

Nenhuma integração está autorizada por este protótipo: persistência futura/Save31, prêmios reais, poderes completos, terceiro jogador, pets, firmware/radio, publicação e instalações continuam fora deste recorte.
