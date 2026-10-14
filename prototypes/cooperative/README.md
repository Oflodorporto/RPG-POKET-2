# Cooperativo local: prova de turnos, autoridade e recuperação

Estado: **protótipo nativo em memória, não integrado às placas**. Base congelada: `5300dada9959c85ff3302508940344d50602036b` (main, pacote clock77tag). Somente arquivos novos neste diretório. Nenhum include no sketch, menu, rádio, CI, produção, Save30, epoch ou rotina de recompensa. Nada publicado.

## O que funciona nesta prova

- Dois endpoints independentes: host e cliente, com intenções, confirmação, ACK, retry, heartbeat e snapshot integral limitado.
- Cada jogador e cada inimigo tem sua vez. Ordem demonstrativa: jogador 1, jogador 2, inimigos vivos em ordem. Mortos são pulados. Rodada seguinte reinicia a fila; onda seguinte começa pelos jogadores vivos.
- Conteúdo provisório: onda 1 com goblin/lobo/esqueleto; onda 2 com javali/lobo/esqueleto; depois Guardião. Os atributos dos inimigos vêm de `rpg::enemySpec`, sem multiplicadores escondidos.
- IDs dos jogadores 1 e 2; inimigos da onda 1: 10/11/12; onda 2: 13/14/15; chefe: 16. Slots internos 0/1 são jogadores, 2 reservado, 3/4/5 inimigos. Não confundir ID visual com índice de slot.
- Um evento confirmado por ação; inclui ator, alvo, tipo, dano/cura, crítico, onda anterior, próxima rodada/vez, fase e estado pós-ação. Cliente verifica evento e estado por replay determinístico antes de apresentá-los.
- Capacidade estática de três slots de jogador, **somente dois jogadores admitidos/testados**. O terceiro não é uma função entregue.

A IA é deliberadamente simples: cada inimigo ataca o jogador vivo com menos HP absoluto; empate favorece jogador 1. A política não é balanceamento final.

## Modelos e limite de fidelidade

`playerProfile(const rpg::Game&, ...)` copia os perfis sem escrever no Game. Preserva classe, nível, aparência, HP/MP, quantidades de poções e atributos derivados de ataque/defesa; não equaliza níveis. Os testes usam personagens sintéticos legítimos construídos por `rpg::create`/`levelUp`, inclusive níveis diferentes. Não acessamos os saves reais do usuário.

Reuso direto de helpers de produção: `effectiveAttack`, `effectiveDefense`, `weaponCriticalChance`, `attacksPerAction`, `fatigueCombatPenalty`, `rollDamage`, `random`, `recovery`, `enemySpec` e `enemyRewardLevel`. O RNG roda em scratch próprio, nunca no Game de campanha. Nenhum `finish`, XP, ouro, conhecimento, captura ou progressão é chamado.

Esta prova **não replica integralmente o combate solo**. A matriz suportada é:

- As quatro classes: ataque básico agregado, crítico, esquiva demonstrativa de 6%, fadiga e atributos importados.
- Defender: guarda demonstrativa de 50% até o próximo ataque recebido, sem custo.
- Poção de vida/mana: fórmula atual de recuperação; consome somente a cópia em RAM.
- `Focus`: ação fictícia comum de teste, 2 MP e +3 dano. Não substitui habilidade de classe e não é uma decisão de produto.
- Não inclui poderes específicos, rage/sacred/surge/controle, smite/brutal adicionais, intenções especiais dos monstros, pets, inventário durável, entrada tardia ou balanceamento por grupo.

Não usar este adaptador como contrato final de fidelidade. Uma integração deve especificar/rejeitar estados transitórios e completar a matriz de poderes antes de anunciar equivalência ao jogo atual. Níveis D&D de 1 a 20 e legados de 1 a 99 são preservados; discriminadores/intervalos inválidos são recusados.

## Autoridade e rede simulada

- Host é a única autoridade. Jogador remoto envia comando com sequência própria, revisão esperada, slot do ator, ação e alvo. Não pode escolher dano ou resultado.
- Um evento confiável fica em voo por vez. Host só avança após ACK de revisão + hash corretos. Repetição de comando aceito devolve snapshot, sem rerrolar. Mesmo ID com outro payload é conflito; sequência antiga/futura é recusada.
- Estado canônico inclui regras, revisão, RNG, counters aceitos, ordem/rodada/onda, atores e recursos. Aparência não altera dano. Animação não altera nenhum deles.
- Versão 1 e magic `0x43504F43`, modo `0x43`, **separados do ClubProtocol PvP v5**. Serialização little-endian explícita, nunca `memcpy` de `Game`/struct para rádio.
- Cabeçalho: magic 4, versão 1, modo 1, tipo 1, peer 1, nonce 8, sequência 4, tamanho 2. Checksum FNV-1a de 4 bytes no final. Estado 185 bytes, snapshot completo 211, evento 227, limite 240. Não há fragmentação ou snapshot parcial instalável.
- Antes de ler estado: validar tamanho, magic, versão, modo, tipo, peer, nonce e checksum. Depois: validar cada limite/invariante; sequência/revisão; continuidade; ator/alvo e simulação completa do evento. Snapshot antigo não regride o estado. Vitória só pode estar na onda do chefe. Contadores aceitos somados não ultrapassam a revisão. Resultado terminal não pode ser reaberto no mesmo nonce.
- `receive(source, packet, now)` pressupõe que um futuro transporte identifica o peer real admitido. IDs 0/1 e checksum não autenticam ninguém. Não há criptografia, resistência a host malicioso, proteção de save adulterado ou promessa antifraude. O nonce é passado pelo harness; uma futura admissão deve gerar identidade de sessão adequada e não reutilizá-la após reboot.
- Handshake de descoberta/admissão, ESP-NOW, RF, callbacks/threads, segurança e capacidade real do payload da versão usada não foram implementados/testados. O harness opera com peers/nonce pré-admitidos.

## Interrupção, reconexão e resultado

Retry: 200 ms, burst de oito tentativas; heartbeat: 500 ms; falta de peer por 1.800 ms pausa. Recuperação usa pedidos/snapshots em intervalos de 500 ms, sem multiplicar filas. O relógio é uint32 com elapsed por subtração, testado através do wrap.

A pausa é de transporte, fora do estado canônico de combate. Nenhum timer inventa vitória/derrota, avança IA ou paga prêmio. Um comando já resolvido fica retido; um comando ainda não aceito pode ser reenviado sem mudar a intenção após snapshot da mesma revisão. Se a revisão mudou, a intenção pendente é cancelada explicitamente como stale; não é adaptada silenciosamente a outro turno.

A reconexão é somente ao **mesmo host ainda vivo**. Cliente recém-reiniciado pode recuperar a cópia da sessão via snapshot. Host reiniciado exige novo nonce; não há migração, eleição, recuperação durável ou garantia de continuar a mesma luta. O teste de reboot recria objetos RAM, não testa NVS nem corte físico de energia.

`Receipt` é apenas confirmação informativa em RAM (nonce, revisão, hash, resultado). Host marca recibo após ACK terminal; cliente após estado terminal válido. ACK perdido não paga nem rola nada novamente. **Não contém/concede recompensas reais e desaparece ao reiniciar.** Não serve de recibo persistente.

HP/MP/poções permanecem exclusivamente no encontro. Nenhum desses recursos é devolvido à campanha. Save30 segue com 160 bytes e sem bytes livres comprovados; migração, sidecar, tickets duráveis e prêmio são decisões separadas não autorizadas aqui.

## Memória e apresentação

Medição nativa g++ 14.2.0 x86-64: Actor 28 bytes; State 192; UiEvent 32; Packet 242 (inclui tamanho); Endpoint 2.800; dois endpoints 5.600 bytes. Cada endpoint tem fila de saída de 8 e fila visual de 8, mais um pacote confiável retido. São objetos fixos; a execução dos testes não fez alocação C++ de heap.

Esses números **não são memória livre no ESP32, pico total, flash ou qualificação de placa**. Há stack temporária para serialização/replay e inicialização; o relatório de stack nativo acompanha o resultado, sem inferir tamanho no Xtensa. Nenhum byte foi adicionado ao firmware porque não há integração.

Fila de saída cheia é contabilizada; evento confiável é retido para retry. Fila visual cheia descarta o evento visual mais antigo e contabiliza overflow, sem bloquear ACK ou reaplicar dano. Snapshot recupera estado, mas não promete reproduzir todas as animações históricas perdidas.

A orientação não é decidida aqui. A saída JSONL é neutra a retrato/paisagem. Na amostra, 27 turnos mostram todos os nove atores, cura, onda nova e chefe. Os dados podem alimentar uma prévia futura; não constituem renderização real do LCD.

## Reproduzir

Na raiz do checkout base com este diretório aplicado:

```sh
prototypes/cooperative/run_tests.sh /tmp/coop-prototype-results
```

O script compila somente dois executáveis de teste focados, otimizados e ASan+UBSan. Não executa suites totais, CI remoto, ESP32, uploads ou rádio. A ferramenta de leak checking foi desabilitada porque LeakSanitizer falha sob o ptrace deste executor; ASan e UBSan passam, e o contador de alocações C++ permanece ativo.

`tests.cpp`: validações de modelo/recursos/turnos; perfis de quatro classes e níveis; igualdade dos 160 bytes serializados de Save30 e do RNG de origem antes/depois; frames inválidos; duplicação/conflito/stale/futuro; ACK errado/perdido; filas cheias; UI lenta; rejoin/reboot limitado; relógio com wrap; derrota e terminal imutável; 96 seeds comparando rede estável e rede com perda/duplicação/reordenação. A rede saudável é restaurada após 30 s simulados e cada execução deve alcançar um terminal comum antes do limite de teste, nunca por timeout de gameplay.

`review_tests.cpp`: 18 regressões independentes incluindo 20.000 datagramas aleatórios, todos os truncamentos e flips de bit de snapshot, recuperação depois de esgotar retry, invariantes terminais e efeito visual adulterado.

Resultado congelado: suíte otimizada com 141.759 checks / 201 cenários; ASan+UBSan com 141.525 / 200 (sem emissão do trace extra). Nas 96 redes perturbadas: 5.140 perdas e 3.065 duplicações; todos os hashes finais iguais às respectivas execuções estáveis. Revisão independente: 18/18 em ambos os builds. Zero alocações C++ na execução instrumentada. São provas desta implementação local, não testes físicos.

## Trace para uma prévia

`turn-trace.jsonl`: uma linha por ação confirmada do host. `actor_id`/`target_id` são IDs de instância; `actors[].slot` são slots internos. `from_wave` e `wave` são 1-based no JSON, enquanto a estrutura interna usa 0-based. `action`: 0 atacar, 1 defender, 2 poção de vida, 3 poção de mana, 4 Focus. `kind`: 0 vazio, 1 jogador, 2 inimigo, 3 chefe. `phase`: 0 jogando, 1 vitória, 2 derrota. `amount` é o valor realmente aplicado; guarda tem amount 0. Em uma troca de onda, o alvo derrotado aparece no evento, enquanto `actors` já mostra a nova onda. Em terminal, `next_actor_id` é apenas o último cursor, não uma autorização para jogar.

Fixtures: mago nível 5 e guerreiro nível 7, construídos pelo modelo atual; seed isolada 123. Primeira rodada de cada onda defende para demonstrar a vez de todos os inimigos. Vitória na revisão 27, hash decimal `180348654`.

## Antes de qualquer integração

1. Fechar política de HP/MP/itens, abandono e host perdido com o usuário.
2. Autorizar uma solução de persistência preservadora separadamente; não usar reset/epoch como contador de sessão.
3. Completar a matriz de combate/classes e conteúdo aprovado; retirar Focus e demais valores demonstrativos conforme decisão.
4. Implementar admissão/transporte real, conferir limites ESP-NOW e rodar em duas placas funcionais.
5. Escolher orientação somente após comparar assets reais; medir stack/heap/PSRAM/flash e toque no hardware.
6. Testar três placas em uma etapa própria antes de habilitar o terceiro participante.
