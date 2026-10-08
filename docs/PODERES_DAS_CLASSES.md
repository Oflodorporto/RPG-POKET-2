# Poderes de classe — 2026.10.08-poderes1

Esta entrega continua a progressão D&D 5e de 2014 / SRD 5.1. Implementa uma primeira fatia de Mago e Paladino, adaptada ao combate individual, mana e duração em turnos do RPG POKET. Não é uma conversão completa das regras de mesa.

## Como usar

Atualize em Título > Configurações > Atualização. Artes do cartão permanecem iguais à versão direcao1. Para testar estas regras, use um herói criado com a progressão nova; heróis anteriores conservam suas técnicas, nível e atributos antigos.

Na cidade/abrigo: Herói > Evolução > Poderes de classe. No combate: Técnicas > Grimório/Poderes. Na dungeon: Menu > Poderes de classe. Anterior/Próximo mostram cada poder; o botão informa custo, nível insuficiente, falta de mana ou usos esgotados. Consultar e voltar não gastam turno nem salvam. Magias são automaticamente disponibilizadas no nível indicado; ainda não há seleção de magias preparadas ou compra de grimórios.

## Mago

| Poder | Nível | Custo | Efeito implementado |
|---|---:|---:|---|
| Mísseis mágicos | 1 | 3 MP | Três dardos de 1d4+1, acertam sem esquiva e ignoram defesa. |
| Mãos flamejantes | 1 | 3 MP | 3d6 de fogo; teste do alvo reduz dano à metade. |
| Escudo arcano | 1 | 3 MP | Reduz em 75% o próximo golpe; ocupa seu turno. |
| Raios abrasadores | 3 | 5 MP | Três raios com testes independentes de acerto; 2d6 por acerto. |
| Bola de fogo | 5 | 7 MP | 8d6 no inimigo atual; teste reduz à metade. |

Teste das chamas: CD = 8 + proficiência + modificador de Inteligência; inimigo usa 1d20+2 como defesa simplificada. Raios usam ataque mágico (d20 + INT + proficiência) contra 10 + DEF/3, natural 1 falha e 20 acerta. Apenas um alvo existe no combate atual; não há dano em área no mapa, resistências elementais ou fogo nos aliados.

O botão ofensivo padrão dos novos Magos usa Mísseis mágicos; defensivo usa Escudo arcano. Os Magos antigos mantêm Raio/Barreira. Escudo de mesa é reação e bônus de CA; aqui é uma ação de proteção, explicitamente adaptada. MP substitui espaços de magia nesta etapa. Recuperação Arcana, tradições e círculos maiores ainda faltam.

## Cavaleiro / Paladino

- Nível 1: Impor as mãos. Reserva de 5 HP por nível; cura até preencher a vida ou esgotar a reserva. Pode usar fora de combate, inclusive na dungeon quando seguro; no combate ocupa o turno. Não custa MP. Poções, vitórias e reinícios não recuperam a reserva.
- Nível 2: a técnica ofensiva existente fica disponível. A punição divina completa com dados de dano, espaços de magia e escolha após acertar ainda não foi convertida; não confundir Investida com a regra de mesa.
- Nível 3: Juramento da Devoção. Escolha explícita e permanente, com confirmação na cidade ou abrigo. Nesta etapa há somente esse juramento; outros não aparecem como escolhas fictícias.
- Devoção: Arma sagrada acrescenta o modificador de Carisma (mínimo 1) ao dano dos próximos três ataques. Ativar ocupa um turno e consome Canalizar Divindade. É adaptação: na mesa o bônus modifica o ataque e a duração é em minutos.
- Devoção: Expulsar profanos faz um morto-vivo perder dois turnos inimigos. Só funciona em Esqueleto, Espectro, Vigia Ossudo e Arconte. Não gasta o uso quando o alvo não é válido. É adaptação sem teste de Sabedoria ou deslocamento de fuga; não há demônios nesta primeira lista.

Os dois poderes de Devoção compartilham **um uso de Canalizar Divindade por descanso**. Não recupera entre encontros; o efeito temporário termina ao sair do combate. O uso e a duração são persistidos para que reiniciar não conceda recurso grátis. Impor as mãos também preserva a reserva consumida.

Descanso concluído no abrigo ou acampamento recupera a reserva e Canalizar. Acampamento iniciado/interrompido, derrota, fuga e poções não recuperam. Nesta adaptação ambos retornam ao finalizar qualquer acampamento, mesmo sem ração; o ganho HP/MP continua 50% sem comida e 100% com comida. É possível iniciar acampamento com HP/MP cheios se os poderes estiverem esgotados.

## Animações e interface

Três projéteis azuis para mísseis; três chamas para mãos/raios; projétil maior com explosão para bola de fogo; brilho dourado para Devoção. Reutilizam as artes aprovadas residentes em PSRAM, sem novos arquivos nem leitura do cartão por quadro. Novos efeitos ofensivos duram entre 1,1 e 1,2 s; entradas feitas na animação não são reaproveitadas no próximo turno.

## Persistência e limites

Save13 continua com 128 bytes e CRC; lê formatos 1–13. Campos anteriores preservados, incluindo acampamento 90/91, eventos 92–99 e evolução 100–108. Campos 109–113: juramento, cura consumida, Canalizar consumido e durações. Reservados 114–123 continuam zerados. Save12 importa com os recursos novos ainda não utilizados, sem recalcular o herói. Saves anteriores à progressão D&D mantêm regras antigas.

Depois de salvar nesta versão, firmware anterior não lê o novo checkpoint: não fazer downgrade sem backup compatível. O agente não grava placa, apaga NVS, muda partições, formata cartão, altera Heltec ou gera .rpg.

## Validação e teste na placa

20 suítes nativas incluem nova class_powers: nível/classe/MP, rejeição sem mutação, usos compartilhados, cura limitada, descanso, duração, alvo inválido e Save13/12. Controlador real testa consulta sem escrita, cancelamento/confirmar juramento, falha de save e retry sem duplicar, mana por lançamento e entrada bloqueada durante animação. Render testa 24 consultas, confirmação e 32 quadros novos, sem alterar saves.

Teste físico ainda necessário: novo Mago no nível 1; verificar raios bloqueados até 3 e bola até 5; novo Paladino cura até esgotar, Devoção no nível 3, Arma sagrada/Expulsar compartilhando uso; descansar e conferir recuperação; retomar após reinício e usar pela dungeon. Sem apagar heróis antigos para testar.

Próximas etapas: punição divina e auras; tradição/recuperação arcana e magias preparadas; subclasses e recursos avançados do Guerreiro/Bárbaro; balanceamento da campanha 1–20. Aeldra ainda precisa de clímax e desfecho para lançamento completo.

## Referências e atribuição

[Classes de 2014](https://www.dndbeyond.com/sources/dnd/basic-rules-2014/classes) e [magias de 2014](https://www.dndbeyond.com/sources/dnd/basic-rules-2014/spells). Níveis e conceitos derivados dessas referências; custos, dano em alvo único e duração acima são adaptações do POKET.

This work includes material taken from the System Reference Document 5.1 (“SRD 5.1”) by Wizards of the Coast LLC and available at https://dnd.wizards.com/resources/systems-reference-document. The SRD 5.1 is licensed under the Creative Commons Attribution 4.0 International License available at https://creativecommons.org/licenses/by/4.0/legalcode.

Publicação confirmada: CI 37766115238 aprovou 20 suítes, compilação ESP32 e Release. Programa 2.636.222 bytes (83%); globais 53.628 bytes (16%). Firmware baixado: 2.636.368 bytes, SHA-256 75621f8a124dfd1cbddee30d5140575946174fcdddac55de59c4391ffe10782a. Pacote de artes compatível conferido contra manifesto. Teste físico pendente.

Continuação implementada em martiais1: Segundo Fôlego, Surto de Ação e Fúria. Veja [PODERES_MARCIAIS.md](PODERES_MARCIAIS.md) para regras e Save14 atuais. A validação e versão acima correspondem à entrega poderes1.
