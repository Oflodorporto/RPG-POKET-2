# Guerreiro e Bárbaro — martiais1

Novos heróis da progressão 1–20 ganham recursos de classe. Heróis anteriores à progressão conservam regras e técnicas antigas. O Clube da Luta conserva seu protocolo e regras próprios nesta etapa.

| Poder | Nível | Comportamento no POKET | Recuperação |
|---|---:|---|---|
| Segundo Fôlego | Guerreiro 1 | Cura 1d10 + nível, limitada ao HP que falta. Ação bônus: mantém o turno e não gasta mana. Pode usar também fora da batalha. | 1 uso por descanso concluído |
| Surto de Ação | Guerreiro 2 | Ativa uma ação extra. A próxima ação normal (ataque, técnica, defesa, poção ou tentativa de fuga) mantém seu turno; depois o inimigo age. Segundo Fôlego não consome essa ação extra. | 1 uso; 2 a partir de 17; no máximo uma ativação por turno |
| Fúria de Batalha | Bárbaro 1 | Ação bônus, sem mana. +2 de dano por ataque até 8, +3 a partir de 9, +4 a partir de 16. Metade do dano físico recebido, arredondada para baixo, após a defesa. Dura três rodadas do inimigo. | 2 usos em 1–2, 3 em 3–5, 4 em 6–11, 5 em 12–16, 6 em 17–19, sem limite em 20 |

O ataque normal inclui os ataques adicionais já implementados por classe. A técnica antiga do Bárbaro aparece como **Golpe brutal** para distinguir do recurso Fúria de Batalha. A Fúria aplica bônus uma vez nessa técnica e uma vez por ataque adicional no ataque normal. Espectro e Arconte causam dano mágico; os demais encontros atuais causam dano físico. A resistência não reduz dano mágico.

## Controles e leitura

Herói > Evolução > Poderes de classe. Em combate: Técnicas > Poderes de classe. Dentro da dungeon: Menu > Poderes de classe. A consulta mostra nível necessário, efeito, usos restantes e duração ativa. Navegar não altera o save. O HUD de combate mostra ação extra pendente ou rodadas de Fúria; o risco do próximo golpe considera a resistência.

Efeitos visuais reutilizam as animações de escudo e Fúria residentes em PSRAM. As artes continuam iguais: não há arquivos novos no cartão nem leituras por quadro.

## Adaptação explícita

Referência: [classes das regras básicas D&D 2014](https://www.dndbeyond.com/sources/dnd/basic-rules-2014/classes). Os níveis, a cura e os limites de usos seguem essa edição. D&D dura Fúria um minuto e possui condições próprias de encerramento, tipos de dano e armadura; POKET usa três rodadas, sua escala de ataque/defesa e a classificação atual dos inimigos. Descanso concluído da cidade ou acampamento repõe os recursos; não há separação entre descanso curto e longo nesta etapa. Surto de Ação permite técnicas e consumíveis do POKET, sem implementar toda a economia de ações de mesa.

Continuam pendentes caminhos/subclasses, Ataque Imprudente, Sentido de Perigo, Indomável, Fúria Persistente e capstone de atributos acima de 20. Não é uma implementação integral do D&D.

## Saves e compatibilidade

Save14 mantém 128 bytes e CRC; lê formatos 1–14. Não muda bytes de camp, eventos, atributos ou juramento. Novos bytes 114–118 guardam Fôlego gasto, Surto gasto, ação extra/ativação no turno, Fúrias gastas e duração. 119–123 continuam reservados e zerados. Importação de Save13 inicia esses recursos sem gasto; herói, itens e demais progressos são preservados. Usos e efeitos são gravados antes da animação. Falha de save pausa o jogo; retry grava o mesmo estado sem repetir o poder. Encerrar a batalha limpa efeitos, não devolve usos. Poções, mudar de tela e reiniciar não recuperam recursos; acampamento só recupera ao concluir.

Após salvar nesta versão, firmware anterior não consegue ler Save14. Não faça downgrade sem backup compatível. Sem gravação automática, apagamento de NVS, formatação, alteração de partições, modificação da Heltec ou pacote .rpg.

## Validação

21 suítes nativas passaram. A suíte martial_powers verifica níveis 1–20, cura, ações adicionais, uso único por turno, resistência física versus mágica com RNG idêntico, risco máximo, três rodadas, nível20 ilimitado, vitória/recompensa única, descanso, legado e save14/13. Controlador real verifica acesso, falha de save e retomada sem duplicar Surto/Fúria. Renderer consulta poderes das quatro classes em níveis 1,2,3,5,17,20 sem modificar save. Teste físico desta versão ainda necessário.

## Cinco testes na placa

1. Em slot livre, criar Guerreiro nível1: Segundo Fôlego deve curar sem gastar o ataque; Surto bloqueado até2.
2. Guerreiro nível2: ativar Surto, atacar duas vezes antes do inimigo; poder fica esgotado. Reiniciar após ativar deve conservar ação extra e gasto.
3. Bárbaro nível1: ativar Fúria, conferir contador três rodadas e resistência; após dois usos, bloqueia até descanso.
4. Completar acampamento ou descanso da cidade e conferir usos repostos; abrir bolsa/tomar poção não repõe.
5. Usar poderes pela dungeon e conferir retorno; testar save antigo sem apagar ou converter manualmente.

This work includes material taken from the System Reference Document 5.1 (“SRD 5.1”) by Wizards of the Coast LLC and available at https://dnd.wizards.com/resources/systems-reference-document. The SRD 5.1 is licensed under the Creative Commons Attribution 4.0 International License available at https://creativecommons.org/licenses/by/4.0/legalcode.

Publicação verificada: CI 37789708521, 21 suítes e ESP32 aprovados. Programa 2.638.098 bytes (83%); globais 53.676 bytes (16%). Firmware baixado 2.638.240 bytes, SHA-256 8001a4ad2c02d7eb6d69d2d3e259ca804c04efc5e6c3953763bb9746c31d53e9. Manifesto e artes compatíveis conferidos. Arquivos em releases/2026.10.08-martiais1; a arte idêntica permanece em releases/2026.10.08-poderes1/artes.pak para evitar duplicar espaço. Teste físico pendente.
