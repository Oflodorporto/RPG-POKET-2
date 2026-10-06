# Estradas e cidades — 2026.10.06-estradas1

A versão anterior `ota1` foi testada na placa pelo usuário: jogo e instalação funcionaram, mas o download exigiu três tentativas. O código anterior descartava os arquivos incompletos. A causa exata da interrupção de rede não foi determinada por logs da placa.

Esta entrega acrescenta cidades com comércio próprio, exploração regional, quatro inimigos inéditos e um teste de viagem com dado. A referência enviada de Knights of Pen & Paper inspirou a apresentação da caixa do dado; o desenho e as regras desta implementação são próprios.

| Região | Nível recomendado | Exploração | Comércio |
|---|---:|---|---|
| Carvalho | 3+ | Bosque: lobo e javali | Equipamento básico, poções a 10/12g e ração a 6g |
| Ruínas | 5+ | Esqueleto e espectro; Guardião após três vitórias locais | Relíquias até Astral com desconto; suprimentos escassos e mais caros |
| Mares | 10+ | Costa: saqueador | Armas de níveis básico/intermediário com desconto; mapa de viagem a 12g |
| Aurora | 18+ | Fortaleza: sentinela | Catálogo completo, armaduras e forja mais baratas; talismã a 25g |

Níveis são recomendações, não bloqueios: a tela de exploração e o mapa avisam quando o destino é perigoso. Os equipamentos continuam com suas exigências de classe e nível. Itens já comprados permanecem na bolsa ao trocar de cidade. Estoque, preço exibido e preço cobrado vêm das mesmas regras.

## Viagem

Ao tocar em Viajar, o jogo sorteia e salva **D20 + Sobrevivência + Sorte** antes de mostrar a animação. A caixa do dado fica sobre o mapa durante 1,8 segundo e depois mostra o resultado, a soma e a dificuldade (CD). O jogador confirma para continuar ou enfrentar o inimigo.

- Sobrevivência inicial: Mago 1, Cavaleiro 3, Guerreiro 3, Bárbaro 5. Ganha um ponto a cada cinco níveis, até 10.
- Sorte inicial: Mago 3, Cavaleiro 2, Guerreiro 2, Bárbaro 1. Ganha um ponto a cada oito níveis, até 8.
- CD = 12 + duas vezes a distância entre regiões na cadeia + nível recomendado da região mais difícil dividido por seis (inteiro).
- Dado 1 sempre provoca encontro; dado 20 sempre garante passagem segura. Nos demais resultados, a soma deve atingir a CD.
- Uma ração dá +2 Sobrevivência; um mapa dá +1 Sobrevivência e +1 Sorte; um talismã dá +2 Sorte. Cada tipo comprado é usado automaticamente, uma unidade por viagem, com limite de nove na bolsa. Bônus aparecem na soma.
- Falha gera um inimigo da faixa de perigo da rota. Vencer ou fugir permite concluir a viagem; perder interrompe a viagem na origem, com a penalidade normal de derrota.
- Encontros na estrada dão recompensas normais, mas não contam como vitórias locais das Ruínas nem como contratos da guilda.
- Reiniciar retoma o dado/encontro já salvo. Retomar uma travessia pode repetir apenas a animação do caminho, sem novo sorteio ou recompensa.

## Artes

Quatro sprites originais gerados com a habilidade imagegen: javali, espectro, saqueador e sentinela. Cada um tem quatro quadros, incluindo poses estáticas e preparação/ataque voltados para a esquerda. Originais completos e quadros de 80×86 estão em `assets/estradas1`; prompts resumidos em `LEIA_ASSETS.txt`. Os fundos de exploração vêm das imagens autorizadas da pasta conceito: bosque, ruínas, costa e fortaleza.

O novo pacote tem 417 recursos e 6.158.160 bytes. É carregado uma vez na PSRAM; o cartão continua desmontado durante o jogo. A Heltec permanece separada.

## Download e saves

O novo atualizador tenta baixar até cinco vezes e retoma blocos completos com HTTP Range. Confere o intervalo devolvido pelo servidor; se ele ignorar Range, recomeça o arquivo com segurança. Um arquivo de identificação associa o parcial ao SHA-256 do pacote. Interrupções preservam os parciais para outra tentativa, inclusive após reiniciar. Arquivos de arte já presentes e verificados não são baixados outra vez. A verificação SHA-256/CRC ocorre antes da instalação. O Wi-Fi fica sem economia de energia durante a instalação confirmada.

**A instalação desta entrega ainda será feita pelo atualizador antigo `ota1`; a melhoria de retomada estará disponível depois que `estradas1` estiver instalada.** Se houver dificuldade nessa primeira passagem, o sketch também pode ser gravado manualmente pelo cabo, com as mesmas opções e sem apagar flash. Não é necessário retirar o cartão para a atualização normal.

O save passa de formato 7 para **8**, mantendo 96 bytes e o diário de dois registros com CRC. Lê e migra os formatos 1 a 7, preservando personagem, raça, roupa, equipamentos, missões, cidade e guilda. Os bytes adicionais guardam a viagem e os suprimentos. Um firmware antigo não lê saves 8: não voltar à versão antiga para continuar um personagem já salvo nesta entrega. Nunca apagar NVS para resolver incompatibilidade.

No manifesto, `save_format: 7` permanece como compatibilidade com o instalador `ota1`. O campo adicional `writes_save_format: 8` informa o formato efetivamente escrito; `reads_save_formats` declara os formatos importáveis. Não são formatos de pacote `.rpg`.

## Verificação e teste na placa

Testes nativos verificam 4.752 viagens, resultados 1/20, atributos e consumo, retomada sem novo dado, derrota/fuga/chegada, importação de save7, preços/estoques, controles reais do sketch, telas e falhas de download/instalação. Compilação e testes físicos são resultados separados. Nenhuma placa foi gravada pelo agente.

Após instalar manualmente ou em Configurações → Atualização, verificar:

1. Os três personagens, equipamentos e progresso anteriores continuam presentes.
2. Explorar cada cidade mostra cenário, inimigo e aviso de nível próprios.
3. Dado mostra bônus e CD; reiniciar durante a rolagem/encontro mantém o resultado.
4. Comprar equipamentos/suprimentos cobra exatamente o valor mostrado; suprimentos são usados uma vez na próxima viagem.
5. Nas próximas atualizações, observar retomada automática e preservação do jogo se o Wi-Fi cair. Não cortar a alimentação durante escrita no cartão ou instalação.
