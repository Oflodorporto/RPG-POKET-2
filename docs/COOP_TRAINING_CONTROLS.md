# Mapa de controles — treino cooperativo

Coordenadas lógicas em paisagem **320×240**, com origem no canto superior esquerdo. Os retângulos são semiabertos: origem inclusiva, limite final exclusivo. A prévia real do renderer acompanha este documento.

## Atualização coopdiag77 · sinal, diagnóstico e reconexão

Este complemento descreve `2026.10.15-coopdiag77` / build `2026101502`, com qualificação de software e [publicação verificadas no estado atual](ESTADO_ATUAL.md). A validação física do toque, orientação e rádio permanece pendente.

- **INFO / VOLTAR:** `(214,2,48,18)`, abre/fecha o diagnóstico local sem parar a rede. Enquanto o painel está aberto, comandos de combate ficam bloqueados; gestos anteriores não atravessam a mudança de tela. A linha LOCAL informa apenas a versão desta placa, não a versão remota.
- **Sinal:** barras por RSSI recebido, idade e estados sem amostra/sem sinal; uma amostra antiga não se torna ping ou prova de comunicação nos dois sentidos. Contadores de envio aceito pela API, conclusão MAC e ACK válido do protocolo são separados.
- **Reconectando...:** overlay central com três pontos animados, somente enquanto o protocolo ainda pode recuperar a mesma sessão. **CANCELAR RECONEXAO** em `(66,116,188,32)` limpa encontro/tokens em RAM e volta ao lobby em paisagem; não reinicia nem repete a batalha antiga. Não concede recompensa nem altera campanha/saves.
- **Retorno central:** após `CONEXAO ENCERRADA` ou `TEMPO DE CONEXAO ESGOTADO`, **VOLTAR A SALA** em `(48,180,224,38)` permite criar/entrar novamente. Nos demais resultados e indisponibilidade, **VOLTAR A GUILDA** mantém a saída em retrato. **SAIR** no topo continua sempre disponível.

### Convite e toque preservados

- O convite continua em protocolo **v2**, compatível com coopfix77 e incompatível com coop77. Atualize as duas placas para os mesmos controles/diagnósticos. O anfitrião decide em **30 segundos desde o primeiro pedido válido**; repetições não reiniciam o prazo. No convidado, a contagem é aproximada e aparece após a confirmação de espera do anfitrião.
- Conexão inicial e sincronização pós-aceite mantêm limites técnicos de **12 segundos**. Expiração do convite, recusa manual e timeout técnico têm mensagens diferentes.
- A escolha de ação, habilidade e alvo em combate **não tem prazo humano enquanto o link estiver saudável**; os limites de perda de conexão continuam.
- Amostras consecutivas idênticas de toque são reunidas sem descartar bordas/mudanças; excesso de eventos ainda cancela o contato de forma segura. Solte o dedo entre comandos. A confiabilidade física exige reteste.
- A saída à Guilda usa limpeza de 350 ms e retorno ao retrato. O gesto iniciado na tela anterior não pode confirmar a saída nem o retorno à sala. **SAIR** permanece durante o combate; a reconexão não retoma uma sessão que já terminou.

## Entrar e sair

- Guilda em retrato: botão “Coop treino” em `(124,218,102,40)`.
- No treino: “Sair” em `(266,2,50,18)`, sempre separado dos alvos e das ações.
- O contato precisa ser solto por 30 ms antes de rearmar. Segurar, arrastar entre regiões ou ter uma leitura inválida não confirma uma ação.
- “Sair” envia o encerramento e mantém uma janela de 350 ms para retransmitir o aviso; depois restaura a Guilda em retrato. Se o aviso não chegar, a outra placa encerra pelo limite de conexão.

## Lobby

- Criar sala: `(16,181,136,38)`.
- Entrar na sala 1: `(168,50,136,27)`.
- Entrar na sala 2: `(168,80,136,27)`.
- Entrar na sala 3: `(168,110,136,27)`.
- Entrar na sala 4: `(168,140,136,27)`.
- Aceitar parceiro: `(16,181,136,38)`.
- Recusar parceiro: `(168,181,136,38)`.

Linhas ausentes/ocupadas não são clicáveis. Classe, nível e código da sala permitem conferir o parceiro. Troca de identidade/token de uma linha invalida o gesto que já estava pressionado, mesmo se o código curto visível coincidir.

## Combate

Faixa inferior, quatro botões:

1. Ataque: `(4,199,75,30)` → tocar no inimigo vivo.
2. Habilidade: `(83,199,75,30)` → “Foco teste” → tocar no inimigo vivo. É a habilidade experimental de 2 MP do treino.
3. Defender: `(162,199,75,30)` → usa a vez com guarda.
4. Itens: `(241,199,75,30)` → poção HP/MP da cópia RAM.

Durante escolha de habilidade, item ou alvo, o quarto botão vira “Cancelar”. Cancelar antes do envio não gasta recurso nem avança turno. Depois do envio, os controles ficam bloqueados até confirmação; não existe gasto especulativo no cliente.

Alvos normais:

- Primeiro: `(185,45,61,50)`.
- Segundo: `(252,70,63,53)`.
- Terceiro: `(185,98,61,50)`.
- Chefe: `(202,50,104,98)`; os outros dois slots ficam vazios.

O contorno indica os alvos válidos para a ação selecionada. Alvo morto, aliado, espaço vazio ou outra vez não recebe ataque. Cada inimigo tem sua vez, com espera visual mínima de 700 ms após a confirmação da revisão anterior; a rede continua recebendo ACKs e fazendo retries durante essa espera.

## Orientação

O framebuffer permanece fisicamente 240×320. A ponte mapeia paisagem `(x,y)` para retrato `(239-y,x)`, e o toque faz o inverso `(y,239-x)`. Não há mudança global de rotação do LCD/FrameBuffer. Ao sair, o desenho e o filtro de toque normais voltam sem aproveitar o dedo ainda pressionado.

O sentido físico e o tamanho confortável de todos os alvos, especialmente “Sair”, ainda precisam ser aprovados nos dois aparelhos.
