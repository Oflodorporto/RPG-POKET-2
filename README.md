# RPG POKET 2.0

RPG para Waveshare ESP32-S3-Touch-LCD-2, SKU 29667 (16 MiB flash, 8 MiB OPI PSRAM, tela 240×320).

## Instalação inicial

Abra `firmware/RPG_POKET_2/RPG_POKET_2.ino` no Arduino IDE. Core Espressif 3.3.12, ESP32S3 Dev Module, flash 16 MB, PSRAM OPI, partição `app3M_fat9M_16MB`, USB CDC ativado. Não apague a flash. Para instalar as artes pelo computador, baixe `artes.pak` da Release e coloque em `/RPGPOKET/artes.pak` num microSD FAT32. No repositorio o pacote fica compactado como `artes.pak.gz`; o workflow o descompacta e valida. A versão inicial do atualizador precisa ser gravada pelo cabo uma única vez.

## Atualizações pela placa

Conecte a uma rede 2,4 GHz. Vá a **Configurações → Atualização → Verificar versão**. Confirme **Instalar** para baixar o firmware e as artes. A placa confere SHA-256, tamanho e CRC do pacote de artes, instala o firmware na partição livre e reinicia. As artes antigas ficam intactas até a nova versão iniciar e validar o novo pacote; então os arquivos temporários e as artes obsoletas são removidos. Os três saves permanecem em NVS.

Sem uma Release publicada, a placa informa que ainda não há versão disponível. Se as artes locais faltarem, é possível baixar as da mesma versão sem regravar o firmware. Não há leituras periódicas do cartão durante o jogo. Não retire o cartão ou a alimentação durante a instalação. FAT32 não garante recuperação de toda falha de energia; não se promete rollback automático de firmware que não inicialize.

## Publicar uma versão

1. Atualize `ReleaseVersion.h`: versão e número crescente `FW_BUILD`.
2. Atualize fontes e pacote `cartao/RPGPOKET/artes.pak`, conservando os identificadores em `AssetCatalog.h`.
3. Faça os testes e envie ao repositório.
4. Em **Actions → Publicar versão → Run workflow**, informe a mesma versão de `FW_VERSION`.
5. O envio de alteracoes a `main` tambem inicia a publicacao automaticamente. O workflow compila, verifica os tamanhos e publica `firmware.bin`, `artes.pak` e `manifest.json` em uma Release. A placa consulta a última Release publicada.

## Estado atual

**Entrega mais recente: Pacote2 — Exploração variada, 2026.10.08-exploracao1.** [Release](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.08-exploracao1) · [Guia e teste físico](docs/PACOTE_2_EXPLORACAO_VARIADA.md) · [Continuidade](docs/CONTINUE_EXPLORACAO1.txt). Achados, baús, mímicos e sucata; Save18 com leitura1–18. As25 suítes, compilação e downloads finais foram verificados; teste físico desta entrega aguarda o usuário. O pacote de artes permanece igual ao do Pacote1. As versões abaixo são histórico.

2026.10.07-lore1: primeira integracao de As Cinzas da Primeira Aurora, abertura com Nara e carta de Maelis, Diario por progresso existente, Pessoas com dialogos locais e atlas de Valdaria. [Uso e limites](docs/REORGANIZACAO_AELDRA.md). Save10/96bytes e artes SD permanecem iguais; a leitura da historia nao altera recompensas ou personagens. Novas regioes, eventos diarios e campanha completa serao etapas posteriores. [Lore](docs/LORE_CANONICA_RPG_POKET_2.md) e [continente](docs/CONTINENTE_VALDARIA.md).

O projeto inclui GFX de terceiros em `src/GFX`; os avisos/licenças existentes acompanham a biblioteca. Nenhuma licença nova é atribuída ao projeto ou às imagens de terceiros sem decisão do proprietário.

## Estradas e cidades

Versao 2026.10.06-estradas1: cidades com niveis e comercio proprios, quatro inimigos originais, exploracao regional e D20 persistente na viagem. Download com retomada e novas tentativas automaticas. [Regras, saves e teste na placa](docs/ESTRADAS_E_CIDADES.md).

A ota1 foi confirmada funcionando na placa pelo usuario. Esta entrega nova exige validacao fisica separada. Saves escritos no formato8; nao voltar ao firmware antigo para continuar estes saves. Sem gravacao automatica ou arquivos .rpg.

## Wi-Fi e relogio

2026.10.06-wifi1: cinco redes salvas, reconexao, indicador RSSI em quatro barras, relogio preto apos um minuto com brilho reduzido, Continuar download e diagnostico HTTPS do GitHub. [Uso e limites do teste](docs/WIFI_RELOGIO_E_DIAGNOSTICO.md).

Firmware e artes mantem saves8 e o pacote estradas1 de417 recursos. Sem gravacao automatica ou .rpg. Testes fisicos de wifi1 confirmados pelo usuario.

## Cripta do Arconte

2026.10.06-dungeon1c: primeira pessoa, dois andares, selo, sete encontros e chefe, cristal por drop/300 ouro, 44 assets originais e saves9 com importacao1..8. [Controles e detalhes](docs/DUNGEON_PRIMEIRA_PESSOA.md). A arte da dungeon acompanha o firmware; artes.pak permanece compativel. Testes fisicos de wifi1 confirmados pelo usuario; dungeon1c confirmada funcionando na placa pelo usuario.

2026.10.07-dia1: hora local e paletas do mundo: [guia dia1](docs/HORA_E_DIA_NOITE.md). Eventos e novos inimigos noturnos continuam planejados.

2026.10.07-cartas1: [carta de Maelis e missao do hipogrifo](docs/CARTAS_E_HIPOGRIFO.md), Save11/128bytes com leitura1–10. Piloto: uma oferta diaria; demais eventos continuam planejados.

## Tela de titulo e jornada

A versao titulo1 abre no titulo em todo reinicio, protege os tres slots na criacao e concentra as configuracoes. Menu > Diario > Objetivo orienta a historia sem modificar os saves. Consulte [guia e testes](docs/TELA_TITULO_E_JORNADA.md).

## Telas do conceito

Abrigo, menu, mapa de Aeldra e Ruinas de Vespera usam as artes fornecidas, com botoes interativos e dados reais. D20 e viagem animados preservados. [Guia e teste na placa](docs/ABRIGO_E_MENU_CONCEITO.md). Versao 2026.10.07-abrigo1.

## Combate, bolsa e acampamento com os conceitos

2026.10.07-paineis1b: seis conceitos adaptados para oito telas, com dados e herois reais. Imagens 240x320 carregadas do SD para PSRAM na inicializacao; sem leitura por quadro. [Guia e teste na placa](docs/NOVAS_TELAS_CONCEITO.md). Atualize firmware e artes por Configuracoes > Atualizacao.

## Evolução e direção de lançamento

2026.10.08-direcao1: novos heróis no nível 1, curva D&D 1–20, atributos e talento Resistente, orientação do objetivo, combate/bolsa/recompensas mais claros. [Evolução e limites da adaptação](docs/EVOLUCAO_DND.md) e [avaliação crítica de lançamento](docs/AUDITORIA_LANCAMENTO.md). Saves anteriores preservados; Save12 não permite downgrade para firmware antigo.

## Grimório e Devoção

2026.10.08-poderes1: cinco magias de Mago, Impor as mãos e Juramento da Devoção com dois poderes e uso compartilhado. [Controles, custos, saves e limites da adaptação](docs/PODERES_DAS_CLASSES.md). Saves13, leitura1–13; heróis antigos preservados. Artes inalteradas.

## Guerreiro e Bárbaro

2026.10.08-martiais1: Segundo Fôlego, Surto de Ação e Fúria com usos por descanso, duração e retomada persistida. [Regras, controles, adaptações e teste na placa](docs/PODERES_MARCIAIS.md). 21 suítes; Save14/leitura1–14; artes inalteradas.

## Revisão para lançamento

2026.10.08-lancamento1 aplica cinco de dez melhorias: intenções de inimigos, preparação de viagem, compra comparativa, recuperação após derrota e guia. [Revisão completa e limites](docs/REVISAO_LANCAMENTO_10_MELHORIAS.md). 22 suítes; save15/leitura1–15; artes compatíveis. A campanha completa ainda está em desenvolvimento.

## A campanha chega a Mares

2026.10.08-mares1: duas missões de Sabela depois da Cripta, nível10+, provas da Mão de Cinza e resgate dos refugiados. [Como jogar e limites](docs/CAMPANHA_MARES.md). 23 suítes, save16/importação1–15, mesmas artes. Aurora e o final ainda estão em desenvolvimento.

## Pacote 1 — Origem e rostos

2026.10.08-origens1: origem por classe, chegada a Carvalho, rostos dos NPCs e primeiro objetivo concreto. [Conteúdo, compatibilidade e testes na placa](docs/PACOTE_1_ORIGEM_E_ROSTOS.md). [Próximos pacotes](docs/ROADMAP_PACOTES_EXPERIENCIA.md).
