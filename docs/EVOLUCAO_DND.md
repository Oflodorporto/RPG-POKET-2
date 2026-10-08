# Evolução por classes — direcao1

Base escolhida: D&D5e de2014/SRD5.1, sem misturar a revisão2024. Cavaleiro=Paladino por escolha explícita do usuário; Mago=Wizard, Guerreiro=Fighter, Bárbaro=Barbarian. XP acumulado e proficiência são compartilhados; recursos e marcos de atributos variam por classe.

Fonte: https://media.wizards.com/2023/downloads/dnd/SRD_CC_v5.1.pdf

A tabela abaixo é referência de desenvolvimento, não uma declaração de que todas as magias, subclasses e auras já estão funcionando.

| Nível | XP acumulado | XP adicional | Prof. | Mago | Cavaleiro/Paladino | Guerreiro | Bárbaro |
|---|---:|---:|---:|---|---|---|---|
| 1 | 0 | 0 | +2 | Magia / recuperacao arcana | Sentido divino / cura | Estilo / segundo folego | Furia / defesa sem armadura |
| 2 | 300 | 300 | +2 | Tradicao arcana | Estilo / magia / punicao | Surto de acao | Ataque imprudente / perigo |
| 3 | 900 | 600 | +2 | Magias de circulo 2 | Juramento / saude divina | Arquetipo marcial | Caminho primitivo |
| 4 | 2700 | 1800 | +2 | Atributos ou talento | Atributos ou talento | Atributos ou talento | Atributos ou talento |
| 5 | 6500 | 3800 | +3 | Magias de circulo 3 | Ataque extra / circulo 2 | Ataque extra: 2 golpes | Ataque extra / movimento |
| 6 | 14000 | 7500 | +3 | Recurso da tradicao | Aura de protecao | Atributos ou talento | Recurso do caminho |
| 7 | 23000 | 9000 | +3 | Magias de circulo 4 | Recurso do juramento | Recurso do arquetipo | Instinto selvagem |
| 8 | 34000 | 11000 | +3 | Atributos ou talento | Atributos ou talento | Atributos ou talento | Atributos ou talento |
| 9 | 48000 | 14000 | +4 | Magias de circulo 5 | Magias de circulo 3 | Indomavel: 1 uso | Critico brutal: 1 dado |
| 10 | 64000 | 16000 | +4 | Recurso da tradicao | Aura de coragem | Recurso do arquetipo | Recurso do caminho |
| 11 | 85000 | 21000 | +4 | Magias de circulo 6 | Punicao aprimorada | Ataque extra: 3 golpes | Furia implacavel |
| 12 | 100000 | 15000 | +4 | Atributos ou talento | Atributos ou talento | Atributos ou talento | Atributos ou talento |
| 13 | 120000 | 20000 | +5 | Magias de circulo 7 | Magias de circulo 4 | Indomavel: 2 usos | Critico brutal: 2 dados |
| 14 | 140000 | 20000 | +5 | Recurso da tradicao | Toque purificador | Atributos ou talento | Recurso do caminho |
| 15 | 165000 | 25000 | +5 | Magias de circulo 8 | Recurso do juramento | Recurso do arquetipo | Furia persistente |
| 16 | 195000 | 30000 | +5 | Atributos ou talento | Atributos ou talento | Atributos ou talento | Atributos ou talento |
| 17 | 225000 | 30000 | +6 | Magias de circulo 9 | Magias de circulo 5 | Surto 2 / indomavel 3 | Critico brutal: 3 dados |
| 18 | 265000 | 40000 | +6 | Maestria em magia | Auras ampliadas | Recurso do arquetipo | Forca indomavel |
| 19 | 305000 | 40000 | +6 | Atributos ou talento | Atributos ou talento | Atributos ou talento | Atributos ou talento |
| 20 | 355000 | 50000 | +6 | Magias de assinatura | Apice do juramento | Ataque extra: 4 golpes | Campeao primitivo |

## Implementado nesta etapa

Novos heróis nível1/0XP; limiares de XP oficiais até20; XP interno é a parcela do nível, e Evolução mostra os valores totais oficiais. HP inicial=dado máximo+modificador de Constituição; crescimento fixo: Mago4+CON, Cavaleiro/Guerreiro6+CON, Bárbaro7+CON. O ganho aumenta HP máximo, sem cura grátis ao subir. Proficiência+2..+6 entra no ataque adaptado. Seis atributos iniciais da matriz padrão por classe, até20; Constituição aumenta HP retroativamente; Força/Inteligência afetam ataque, Destreza defesa, Sabedoria sobrevivência, INT/Carisma mana de Mago/Paladino. Mana, defesa e ataque ainda usam a escala própria do jogo: não são slots de magia ou CA de D&D.

Marcos4,8,12,16,19 liberam2pontos para +2 em um atributo ou +1 em dois. Guerreiro também6e14. Alternativa implementada: talento Resistente (Tough), uma vez, +2HP por nível e nos níveis futuros. Gastar um ponto impede trocar aquele mesmo marco pelo talento; pontos e escolha persistem. Evolução acessível em Herói> Evolução (botão sob o retrato). Novos Paladinos só usam técnica ofensiva a partir do nível2; demais ações antigas são adaptações do RPGPOKET.

Ataques extras: classes marciais2no5; Guerreiro3no11e4no20. Nesta etapa, multiplicam o dano do ataque normal com um único teste de esquiva; não são rolagens independentes de D&D. Crítico do Bárbaro recebe incremento nos marcos9/13/17; é adaptação em pontos de dano, não dados de arma. Magias/círculos ficam na referência; grimório ainda não existe.

Primeiros encontros dos novos heróis em Carvalho são Goblin/Lobo reduzidos (níveis1/2), com HP/ATQ coerentes com a HUD; depois voltam à fauna anterior. XP de combate dos novos heróis=10xbase; ouro permanece igual. Contratos dos novos heróis dão percentual do intervalo oficial capturado ao aceitar. Esse ritmo precisa de teste de campanha, não é balanceamento final.

## Preservação dos heróis antigos

Save12 mantém128bytes e CRC. Lê1..12. Bytes90/91(acampamento),92..99(eventos), destinos e namespaces preservados. Bytes100..108 guardam modo, talento, pontos e atributos. Arquivos/saves antigos importados conservam nível, XP, HP, equipamento e a curva anterior; não ganham nem perdem atributos silenciosamente. Não foi feita migração global de curva. Na Evolução aparece a informação de regras antigas. Novas escolhas são salvas pelos checkpoints existentes e pausam em erro de escrita. Firmware anterior não lêSave12: não fazer downgrade depois de salvar nesta versão sem backup compatível.

## Ainda falta para uma conversão completa

Escolha de subclasses e juramentos; poderes do juramento, auras e cura de Paladino; grimório, lista de magias e slots; surto de ação e indomável do Guerreiro; usos/duração/resistências da fúria; talentos além de Resistente; ataques extras com rolagens e animações individuais. Nenhum marco de referência aparece como botão executável antes de seu sistema existir. A tabela serve como contrato para as próximas etapas.

## Atribuição

This work includes material taken from the System Reference Document5.1 (“SRD5.1”) by Wizards of the Coast LLC and available at https://dnd.wizards.com/resources/systems-reference-document. The SRD5.1 is licensed under the Creative Commons Attribution4.0 International License available at https://creativecommons.org/licenses/by/4.0/legalcode.
Traduções resumidas e adaptações de combate/progressão para RPGPOKET por este projeto. Dungeons&Dragons não é a marca deste jogo.
