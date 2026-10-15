# Guia rápido · RPG POKET 2.0

[Voltar à apresentação](../README.md) · [Estado publicado](ESTADO_ATUAL.md) · [Índice de documentação](INDICE.md)

Este guia descreve a versão **2026.10.15-coopdiag77 / build 2026101502**, publicada em 15/10/2026. Documentos de entregas antigas podem descrever outro formato de save ou outro fluxo de publicação.

## Antes de começar

- Placa **Waveshare ESP32-S3-Touch-LCD-2, SKU 29667**.
- Flash de **16 MiB**, PSRAM **OPI de 8 MiB**, tela **240 × 320**.
- MicroSD **FAT32**, cabo USB de dados e alimentação estável.
- Para atualizar pela placa, rede Wi-Fi de **2,4 GHz**.
- Baixe código e artes da [Release coopdiag77](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.15-coopdiag77). Não misture arquivos de tags diferentes.

Os saves ficam em NVS na placa, não no cartão. Copiar o microSD não é um backup dos três slots. Evite trocar partições ou usar ferramentas de apagamento total como tentativa de recuperação.

## Compatibilidade dos saves

| O que está instalado | Caminho | Efeito esperado sobre os saves |
| --- | --- | --- |
| Primeira instalação, sem jogo anterior | USB | Cria a geração atual de saves. |
| touch77 ou anterior / Save1…28 | **USB uma vez, sem apagar toda a flash** | Reinicia os três slots antigos uma única vez; não importa Save1…28. Wi-Fi e artes ficam fora desse reset. |
| scenes77, scale77, camp77 / Save29 | Atualização compatível pela placa | Preserva os três slots. Migração para Save30 só no próximo save normal. |
| explore77, immersion77, story77, clock77, pvp77, shop77, coop77, coopfix77 / Save30 | Atualização compatível pela placa | Mantém Save30 e época 1, sem novo reset. |

A versão atual **grava Save30 e lê Save29/30**. O leitor e o instalador não migram o save por conta própria. A compatibilidade de leitura dos codecs antigos **não autoriza downgrade de firmware**.

Se você está em touch77 ou anterior e precisa manter aquele progresso, **não avance esperando importação**. O reset dos três slots faz parte da fronteira de compatibilidade. O [registro da transição para Save29](RELEASE_SCENES_7_7_NOTAS.txt) explica o motivo; os dados de versão e leitura Save29-only desse registro são históricos, não o contrato do coopdiag77.

## Instalar pelo USB

1. Abra a [tag da versão atual](https://github.com/Oflodorporto/RPG-POKET-2/tree/v2026.10.15-coopdiag77), baixe o código-fonte completo e extraia a pasta inteira.
2. Abra `firmware/RPG_POKET_2/RPG_POKET_2.ino` no Arduino IDE, mantendo todos os cabeçalhos e arquivos do sketch ao lado dele.
3. Use o core **Espressif ESP32 3.3.12** e a placa **ESP32S3 Dev Module**.
4. Configure:
   - Flash **16 MB**, modo **QIO**.
   - PSRAM **OPI**.
   - CPU **240 MHz**.
   - Partição **app3M_fat9M_16MB**: 16 MB de flash / 3 MB APP / 9,9 MB FATFS.
   - USB CDC ativado; USB mode **Hardware CDC and JTAG**.
   - **Erase All Flash: Disabled / EraseFlash=none**.
5. Compile e envie pelo cabo. Não execute `erase_flash` nem sobrescreva a partição NVS.
6. Instale as artes conforme a próxima seção e confira a versão exibida, os três slots e o funcionamento do toque.

O arquivo `firmware.bin` da Release é uma imagem de aplicação usada pelo atualizador. Este guia **não orienta gravá-lo diretamente no endereço zero** nem o trata como imagem completa de flash.

### Artes no microSD

Com o aparelho desligado e o cartão fora dele, copie o `artes.pak` final da mesma Release para:

```text
/RPGPOKET/artes.pak
```

O pacote tem **666 entradas, 7.470.928 bytes** e CRC32 do conteúdo `9b4bcfc0`. A cópia deve ser do `.pak` final, não de um arquivo compactado renomeado. Recoloque o cartão antes de ligar. Após a instalação USB compatível, a opção Atualização também pode reparar artes ausentes ou inválidas da versão instalada.

Há arquivos compactados de origem no repositório. `tools/unpack_art.py` reconstrói o pacote para desenvolvimento; para instalar, prefira o arquivo final conferido da Release.

## Atualizar pelo Wi-Fi

1. Conecte a uma rede **2,4 GHz**.
2. Abra **Configurações → Atualização → Verificar versão**.
3. Confira a versão oferecida e toque em **Instalar** para autorizar o download.
4. Mantenha alimentação estável e não retire o cartão até a conclusão e o reinício.

O instalador verifica tamanho, SHA-256 e o CRC das artes; escreve o programa na partição de aplicação livre. Downloads completos com falha de leitura podem ser revalidados sem novo download. Há uma tentativa de remontagem de recuperação do SD a 1 MHz, reiniciando a validação integral. Após sucesso, temporários são removidos; versões anteriores das artes e backup são retidos, então reserve espaço adicional no cartão.

O cliente touch77 recusa a passagem para a geração Save29/30: use USB nesse primeiro salto. Não contorne verificações de compatibilidade, integridade ou HTTPS. SHA-256 verifica integridade; não é uma assinatura própria do autor.

**Limites:** FAT32 não garante recuperação de toda falha de energia. Não há promessa de rollback automático de um firmware que não inicialize. Testes OTA em host não demonstram instalação física, comportamento real do cartão ou tolerância a cortes na placa.

## Controles e primeiros passos

- **Toque e solte:** os comandos são acionados por toque; solte o dedo antes do próximo. Não conte com gestos de arrastar como controle da dungeon.
- **Carvalho, página 1:** Conversar, Loja, Bolsa e Explorar. **Mais** abre a página 2.
- **Carvalho, página 2:** Herói, Guilda, Acampar e Mapa. **Voltar** retorna à página 1. Menu está nas duas.
- **Objetivo:** toque no painel do objetivo; o Diário ajuda a acompanhar a história.
- **Dungeon:** seis botões para avançar, recuar, deslocar para os lados e girar 90°. Toque na cena para interagir ou atacar quando a ação estiver disponível.
- **Combate:** confira HP, MP, intenção do inimigo e custo antes de escolher ataque, defesa ou poder.
- **Descanso e viagem:** observe provisões, custo de partida, fadiga e pernoites antes de confirmar.
- **Três slots:** Continuar retoma o último slot válido; **Menu → Personagens** permite escolher outro. A criação protege os slots existentes.

### Quando o jogo salva

As transições relevantes gravam o progresso automaticamente pelo journal: combates, ações da dungeon, saque, compras, descanso e viagem, entre outras. Navegar por menus não equivale a executar uma ação de jogo.

Se aparecer **Tentar salvar**, mantenha a placa ligada e use a recuperação oferecida. Não presuma que uma vitória ou recompensa ainda em memória já está durável. Testes simulados sustentam crédito no máximo uma vez, não gravação atômica ou exatamente uma vez em toda falha de energia.

## Treino cooperativo experimental · coopdiag77

Use **coopdiag77 nas duas placas**, `2026.10.15-coopdiag77` / build `2026101502`. O protocolo de convite v2 continua compatível com coopfix77 e incompatível com coop77; use a versão atual em ambas para obter os novos controles e diagnósticos. A instalação compatível mantém os saves; a publicação não atualiza os aparelhos automaticamente. [Estado e hashes verificados](ESTADO_ATUAL.md).

1. Na Guilda, abra **Coop treino** nas duas placas. Uma cria a sala; a outra toca na sala correta.
2. O anfitrião confere o parceiro e escolhe **ACEITAR** antes de acabar a contagem de 30 segundos. O convidado vê um prazo aproximado; retransmissões não renovam o prazo do anfitrião.
3. **CONVITE EXPIRADO** indica falta de aceite dentro do prazo; **PEDIDO RECUSADO** indica recusa explícita. **TEMPO DE CONEXAO ESGOTADO** corresponde ao limite técnico de 12 segundos na conexão inicial/sincronização. Siga a tela para pedir entrada novamente; no timeout técnico, o botão central **VOLTAR A SALA** retorna ao lobby.
4. Durante o combate, escolher habilidade ou alvo não tem prazo humano com o link saudável. Uma interrupção prolongada do link ainda pode encerrar o treino.
5. O rodapé mostra intensidade recebida ou ausência de sinal; **INFO** mostra a idade da amostra e abre o diagnóstico local. **VOLTAR** fecha o painel. A versão da outra placa não é inferida e não há medição de ping.
6. Durante **Reconectando...**, os pontos animam enquanto o protocolo tenta recuperar a mesma sessão. **CANCELAR RECONEXAO** descarta o encontro em RAM e volta à criação/entrada de sala. Depois de encerramento definitivo não há retomada da sessão antiga.
7. Após conexão encerrada ou timeout técnico, toque **VOLTAR A SALA**. Nos demais resultados, use **VOLTAR A GUILDA**. **SAIR** continua disponível no topo; ao sair à Guilda, confira o retorno ao retrato antes de entrar novamente.

O treino usa cópias em RAM, sem XP, ouro, recompensas ou mudanças nos personagens da campanha. Save30/160, leitura 29/30, época 1 e rações até 10 permanecem, sem novo reset. A qualificação de software não substitui o reteste físico do toque e rádio. [Mapa dos controles](COOP_TRAINING_CONTROLS.md) · [Roteiro físico](COOP_TRAINING_HARDWARE_CHECKLIST.md).

## PvP entre duas placas

Acesse **Carvalho → Mais → Guilda → Clube**, com cadastro na Guilda e personagem de nível 5 ou superior. Use **coopdiag77 nas duas placas**. O protocolo v5/52 bytes não é compatível com o v4/44 bytes. Não há troca de protocolo durante a partida.

- Antes do início, uma interrupção mantém a devolução integral existente.
- Depois do início, sem prova confirmada do resultado, aparece **NAO CONFIRMADO**: sem prêmio de vitória, e a entrada utilizada permanece consumida.
- Desistir funciona também no turno adversário.
- Em desistência simultânea, prevalece a solicitação de quem detém o turno acordado; o outro pode ganhar mesmo tendo pedido desistência.
- Não há promessa de reconexão, consenso durável ou proteção contra firmware adulterado.

O fluxo tem testes host e compilação com ESP-NOW/FreeRTOS real. **O duelo por rádio entre duas placas físicas ainda precisa ser validado nesta versão.**

## Se algo não funcionar

- **Artes ausentes/inválidas:** confira FAT32, caminho, arquivo final e versão. Use o reparo de artes da mesma versão quando disponível.
- **Manifesto recusado:** confira a geração instalada e o caminho USB/OTA correto; não force a instalação.
- **Falha no download/SD:** siga o diagnóstico da tela, mantenha a alimentação e evite apagar a flash como tentativa.
- **Sala coop só aparece de perto ou conexão interrompe:** abra INFO nas duas placas e registre as telas no momento da falha, distância aproximada e alimentação. Não altere potência/canal/sleep nem apague saves para tentar recuperar; [roteiro de diagnóstico](COOP_TRAINING_HARDWARE_CHECKLIST.md).
- **Problema persistente:** [abra uma issue](https://github.com/Oflodorporto/RPG-POKET-2/issues) com versão/build, placa e passos para reproduzir. Não publique senha Wi-Fi, tokens ou informações pessoais.

[Hashes e evidências da versão](ESTADO_ATUAL.md) · [Guias detalhados e registros históricos](INDICE.md)
