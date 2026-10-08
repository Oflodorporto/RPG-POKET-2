# Pacote 1 — Origem e rostos

Versão técnica: `2026.10.08-origens1`, build `2026100806`. Publicado em 08/10/2026. As 24 suítes e a compilação final passaram no GitHub (run 37840225760). Firmware, artes e manifesto publicados foram baixados e seus tamanhos e SHA-256 conferidos; CRC e catálogo das artes também passaram. Validação física desta versão aguarda o usuário.

## A experiência

Depois da criação de raça, classe e roupas, novos personagens recebem oito páginas: três de passado e motivação, quatro da chegada a Carvalho e uma de orientação antes do tutorial. O texto permanece parado para leitura; Voltar e Avançar trocam a página. Pular na primeira página leva ao tutorial, sem concluir o aprendizado automaticamente.

- **Mago:** trabalha para pagar as aulas; parte porque o mestre está perdendo as lembranças.
- **Cavaleiro/Paladino:** aprendeu a proteger viajantes; perdeu o posto ao se recusar a abandonar uma família. Busca trabalho e um juramento digno de seguir, sem impor o juramento de nível 3.
- **Guerreiro:** guarda de caravanas sem trabalho após o fechamento da estrada; quer sustentar a família e reabrir a rota.
- **Bárbaro:** sua comunidade perdeu a horta e os animais fugiram da cinza; busca ajuda para proteger sua casa.

As quatro raças têm o mesmo acesso à história e à campanha. O protagonista importa pelo que faz e por suas relações, sem linhagem secreta ou poderes escolhidos automaticamente. O primeiro objetivo é aprender os controles, preparar a bolsa e falar com Elarin e Borin. A abertura apresenta também um próximo poder realmente implementado da classe, quando o herói está no nível 1.

Personagens anteriores recebem apenas as cinco páginas comuns da chegada ao rever o início. Não lhes é atribuído um novo passado. Releitura: **Menu > Diário > Objetivo > Relembrar o início**. Essa consulta não escreve saves, não entrega recompensas e não reinicia o tutorial.

## Rostos e identidade

O pacote inclui 17 retratos individuais em 64 × 64, com paletas de 128 cores. Os interlocutores implementados aparecem em Pessoas/Conversar, abertura, Guilda, taverna, Clube e carta de Maelis, além dos resultados narrativos de Mares. O nome e a função continuam sendo texto real. Narração é marcada como NARRADOR; o passado como VOCE / ANTES DA JORNADA; carta de Maelis continua identificada como carta.

Vaelor permanece sem rosto/nome revelado antes da descoberta na Cripta. Odran e Anwen possuem assets prontos, mas suas futuras cenas não foram antecipadas ou implementadas por este pacote.

## Memória e compatibilidade

As fontes PNG permanecem em `assets_conceito/retratos_npcs_v1`. O jogo usa dados compactos no pacote `artes.pak`, não os PNGs originais. O acréscimo foi 73.984 bytes; o pacote completo tem 6.846.544 bytes e 459 entradas, CRC `19aa0797`. Os bytes de todas as artes anteriores foram preservados. O carregamento validado na inicialização já existente coloca as imagens em PSRAM; não há leitura de cartão por quadro/conversa. A alternativa de arte reduzida conserva os nomes, funções e textos quando o SD estiver ausente ou inválido.

Save17 continua com 128 bytes e CRC no offset124. Lê formatos1–17. Byte122 guarda a presença de origem (0/1); byte123 guarda página0–7 ou8 para abertura concluída. Formatos1–16 importam ambos como zero, sem recalcular herói, itens, recursos, progressão ou campanha. Avançar/voltar na abertura de um personagem novo grava a página por ação do usuário; uma falha pausa para tentar salvar o mesmo estado. Reiniciar e Continuar retoma essa página; após concluir/pular, retoma o tutorial. Consultas e releituras não gravam.

Firmware antigo não lê save17: não fazer downgrade após gravar novos checkpoints sem backup compatível. IDs dos locais, enumerações, partições e regras de combate/viagem foram preservados. Não houve gravação automática de placa, limpeza de NVS, formatação de SD ou geração de `.rpg`.

## Teste na placa

1. Em um slot vazio, criar um herói; ler, voltar, avançar e conferir sua motivação. Repetir para as quatro classes quando conveniente.
2. Reiniciar no meio da abertura; na tela de título, Continuar deve retomar a página salva. Pular leva ao tutorial, sem batalha/recompensa automática.
3. No personagem antigo, conferir nível, ouro, itens e campanha; rever a chegada, sem origem nova nem repetição obrigatória do tutorial.
4. Conversar com Nara/Elarin/Borin e demais pessoas conhecidas; abrir Guilda, Clube, carta e resultados de Mares; conferir rostos, nomes, texto e toques.
5. Conferir mapa, animação D20 e deslocamento. Testar ausência do cartão somente com a placa desligada ao removê-lo; a história deve continuar legível com arte reduzida.

## Próximos pacotes

1. **Origem e rostos** — esta entrega.
2. **Exploração variada** — encontros, ouro, objetos úteis/inúteis e baús com mímico raro.
3. **Trancas e gazuas** — força, ferramentas e decisões de risco.
4. **Progressão recompensadora** — curva de níveis, economia e escolhas de classe.
5. **Campanha completa** — Aurora, Odran, Coração do Véu e desfechos.
6. **Mundo vivo** — mais eventos e ciclo do mundo.
7. **Qualidade e facilidade** — estabilidade e testes com novos jogadores.
8. **Lançamento** — documentação, créditos, licenças e apresentação.

Esses nomes são os nomes de produto. As versões técnicas continuam únicas e crescentes para o atualizador. Pacotes futuros não estão incluídos nesta entrega.
